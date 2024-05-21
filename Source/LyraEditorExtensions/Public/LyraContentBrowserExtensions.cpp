#include "LyraContentBrowserExtensions.h"
#include "ContentBrowserDelegates.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Factories/Texture2dFactoryNew.h"

#define LOCTEXT_NAMESPACE "LyraEditorExtensions"

DECLARE_LOG_CATEGORY_EXTERN(LogLyraCBExtensions, Log, All);
DEFINE_LOG_CATEGORY(LogLyraCBExtensions);

//////////////////////////////////////////////////////////////////////////

static FContentBrowserMenuExtender_SelectedAssets ContentBrowserExtenderDelegate;
static FDelegateHandle ContentBrowserExtenderDelegateHandle;

//////////////////////////////////////////////////////////////////////////
// FContentBrowserSelectedAssetExtensionBase

struct FContentBrowserSelectedAssetExtensionBase
{
public:
	TArray<struct FAssetData> SelectedAssets;

public:
	virtual void Execute() {}
	virtual ~FContentBrowserSelectedAssetExtensionBase() {}
};

//////////////////////////////////////////////////////////////////////////
// FCreateSpriteFromTextureExtension

#include "IAssetTools.h"
#include "AssetToolsModule.h"

enum ETextureChannel : uint8
{
	Red,
	Green,
	Blue,
	Alpha,
};

struct FTextureChannelPixelInfo
{
	FTextureChannelPixelInfo(TEnumAsByte<ETextureChannel> InChannel, const int32 InWidth, const int32 InHeight)
		: Channel(InChannel)
		, Width(InWidth)
		, Height(InHeight)
		, bContainsGrayPixels(false)
	{
		PixelsAsGrayscale.Reserve(Width * Height);
	}

	FTextureChannelPixelInfo(const uint8 InChannel, const int32 InWidth, const int32 InHeight)
		: Channel(InChannel)
		, Width(InWidth)
		, Height(InHeight)
		, bContainsGrayPixels(false)
	{
		PixelsAsGrayscale.Reserve(Width * Height);
	}

public:
	void AddGrayscalePixel(const uint8* TextureChannelBytePtr)
	{
		const uint8 ChannelValue = *TextureChannelBytePtr;
		PixelsAsGrayscale.Add(FColor(ChannelValue, ChannelValue, ChannelValue, 0));
		if (!bContainsGrayPixels && (ChannelValue != 0 && ChannelValue != 255))
		{
			bContainsGrayPixels = true;
		}
	}

	FString GetChannelName() const
	{
		switch (Channel)
		{
		case ETextureChannel::Red: return TEXT("Red");
		case ETextureChannel::Green: return TEXT("Green");
		case ETextureChannel::Blue: return TEXT("Blue");
		case ETextureChannel::Alpha: return TEXT("Alpha");
		default: return TEXT("Unknown");
		}
	}

	uint8 GetChannel() const
	{
		TArray<uint8> ChannelMapBGRA = { 2, 1, 0, 3 };
		return ChannelMapBGRA[Channel];
	}
	
	TEnumAsByte<ETextureChannel> Channel;
	int32 Width;
	int32 Height;
	TArray<FColor> PixelsAsGrayscale;
	bool bContainsGrayPixels;
};

struct FExtractChannelsFromTextureExtension final : public FContentBrowserSelectedAssetExtensionBase
{
	FExtractChannelsFromTextureExtension()
	{
	}

	static bool ShouldExtractChannelsFromTexture(const UTexture2D* Texture)
	{
		const TEnumAsByte<TextureCompressionSettings> CompressionSettings = Texture->CompressionSettings;
		const bool bIsGrayscale = CompressionSettings == TC_Grayscale;
		const bool bIsNormalMap = CompressionSettings == TC_Normalmap;
		return !bIsGrayscale && !bIsNormalMap;
	}
	
	static void ExtractChannelsFromTextures(const TArray<UTexture2D*>& Textures)
	{
	    for (UTexture2D* Texture : Textures)
	    {
	    	if (ShouldExtractChannelsFromTexture(Texture))
	    	{
	    		ExtractChannelsFromTexture(Texture);
	    	}
	    }
	}

	static void GetTexturePixelChannelInfos(UTexture2D* Texture, TArray<FTextureChannelPixelInfo>& OutChannelPixelInfos)
	{
		FTextureSource& TextureSource = Texture->Source;
		verify(TextureSource.IsValid());
		
		FImage SourceMip0;
		verify(TextureSource.GetMipImage(SourceMip0, 0));

		const uint8* RawDataPtr = TextureSource.LockMipReadOnly(0);
		const int64 BytesPerPixel = TextureSource.GetBytesPerPixel();
		const int64 Width = TextureSource.GetSizeX();
		const int64 Height = TextureSource.GetSizeY();
		const int64 NumPixels = Width * Height;

		TArray<FTextureChannelPixelInfo> ChannelPixelInfos;
		for (int i = 0; i < 4; i++)
		{
			FTextureChannelPixelInfo ChannelInfo = FTextureChannelPixelInfo(i, Width, Height);
			ChannelPixelInfos.Add(ChannelInfo);
		}

		for (int64 Y = 0; Y < Height; ++Y)
		{
			for (int64 X = 0; X < Width; ++X)
			{
				const int64 PixelOffset = (X + Y * Width);
				const int64 PixelByteOffset = PixelOffset * BytesPerPixel;

				// Point to the start of the pixel data in memory
				const uint8* PixelPtr = RawDataPtr + PixelByteOffset;
            	
				for (FTextureChannelPixelInfo& ChannelInfo : ChannelPixelInfos)
				{
					// Point to this channel's byte in memory
					const uint8* PixelColorChannelBytePtr = PixelPtr + ChannelInfo.Channel; // @TODO: check math on this
					
					ChannelInfo.AddGrayscalePixel(PixelColorChannelBytePtr);
				}
			}
		}
		
		Texture->Source.UnlockMip(0);
		OutChannelPixelInfos = ChannelPixelInfos;
	}

	static void ExtractChannelsFromTexture(UTexture2D* Texture)
	{
		TArray<FTextureChannelPixelInfo> ChannelPixelInfos;
		GetTexturePixelChannelInfos(Texture, ChannelPixelInfos);
		
		const FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		const FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		TArray<UObject*> ObjectsToSync;

		for (FTextureChannelPixelInfo& ChannelInfo : ChannelPixelInfos)
		{
			if (!ChannelInfo.bContainsGrayPixels)
			{
				continue;
			}
			
			FString AssetName;
			FString PackageName;
			const FString DefaultSuffix = FString::Printf(TEXT("_Channel%d"), ChannelInfo.GetChannel());
			AssetToolsModule.Get().CreateUniqueAssetName(Texture->GetOutermost()->GetName(), DefaultSuffix, PackageName, AssetName);
			const FString PackagePath = FPackageName::GetLongPackagePath(PackageName);
			UTexture2DFactoryNew* Texture2DFactory = NewObject<UTexture2DFactoryNew>();
			if (UObject* NewAsset = AssetToolsModule.Get().CreateAsset(AssetName, PackagePath, UTexture2D::StaticClass(), Texture2DFactory))
			{
				UTexture2D* NewTexture = CastChecked<UTexture2D>(NewAsset);
				const uint8* RawData = reinterpret_cast<uint8*>(ChannelInfo.PixelsAsGrayscale.GetData());
				NewTexture->Source.Init(ChannelInfo.Width, ChannelInfo.Height, 1, 1, ETextureSourceFormat::TSF_BGRA8, RawData);
				NewTexture->MipGenSettings = Texture->MipGenSettings;
				NewTexture->LODGroup = Texture->LODGroup;
				NewTexture->CompressionSettings = TC_Grayscale;
				NewTexture->CompressionNoAlpha = true;
				NewTexture->UpdateResource();
				ObjectsToSync.Add(NewAsset);
			}
		}
		
		if (ObjectsToSync.Num() > 0)
		{
			ContentBrowserModule.Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
	
	virtual void Execute() override
	{
		// Extract channel from any selected textures
		TArray<UTexture2D*> Textures;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& AssetData = *AssetIt;
			if (UTexture2D* Texture = Cast<UTexture2D>(AssetData.GetAsset()))
			{
				Textures.Add(Texture);
			}
		}

		ExtractChannelsFromTextures(Textures);
	}
};

//////////////////////////////////////////////////////////////////////////
// FLyraContentBrowserExtensions_Impl

class FLyraContentBrowserExtensions_Impl
{
public:
	static void ExecuteSelectedContentFunctor(TSharedPtr<FContentBrowserSelectedAssetExtensionBase> SelectedAssetFunctor)
	{
		SelectedAssetFunctor->Execute();
	}

	static void CreateTextureActionsSubMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets, uint8 CanCreateFlags = 0)
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("TextureActionsSubMenuLabel", "Texture Actions"),
			LOCTEXT("TextureActionsSubMenuToolTip", "Texture2D-related actions for this texture."),
			FNewMenuDelegate::CreateStatic(&FLyraContentBrowserExtensions_Impl::PopulateTextureActionsMenu, SelectedAssets, CanCreateFlags),
			false,
			FSlateIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ClassIcon.Texture2D"))
		);
	}

	static void PopulateTextureActionsMenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> SelectedAssets, uint8 CanCreateFlags = 0)
	{
		const FName StyleSetName = FAppStyle::GetAppStyleSetName();
	
		// Extract channels
		const TSharedPtr<FExtractChannelsFromTextureExtension> ChannelExtractorFunctor = MakeShareable(new FExtractChannelsFromTextureExtension());
		ChannelExtractorFunctor->SelectedAssets = SelectedAssets;

		const FUIAction Action_ExtractChannelsFromTextures(
			FExecuteAction::CreateStatic(&FLyraContentBrowserExtensions_Impl::ExecuteSelectedContentFunctor, StaticCastSharedPtr<FContentBrowserSelectedAssetExtensionBase>(ChannelExtractorFunctor)));
		
		MenuBuilder.AddMenuEntry(
			LOCTEXT("CB_Extension_Texture_ExtractChannels", "Extract Channels"),
			LOCTEXT("CB_Extension_Texture_ExtractChannels_Tooltip", "Extracts color channels into their own greyscale textures"),
			FSlateIcon(StyleSetName, "LevelEditor.Tabs.Layers"),
			Action_ExtractChannelsFromTextures,
			NAME_None,
			EUserInterfaceActionType::Button);

	}

	static TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& SelectedAssets)
	{
		TSharedRef<FExtender> Extender(new FExtender());

		// Run thru the assets to determine if any meet our criteria
		bool bAnyTextures = false;
		for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FAssetData& Asset = *AssetIt;
			bAnyTextures = bAnyTextures || (Asset.AssetClassPath == UTexture2D::StaticClass()->GetClassPathName());
			// TODO: Check for greyscale textures etc
		}

		if (bAnyTextures)
		{
			uint8 CanCreateFlags = 0;
			
			// Add the sprite actions sub-menu extender
			Extender->AddMenuExtension(
				"GetAssetActions",
				EExtensionHook::After,
				nullptr,
				FMenuExtensionDelegate::CreateStatic(&FLyraContentBrowserExtensions_Impl::CreateTextureActionsSubMenu, SelectedAssets, CanCreateFlags));
		}

		return Extender;
	}

	static TArray<FContentBrowserMenuExtender_SelectedAssets>& GetExtenderDelegates()
	{
		FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
		return ContentBrowserModule.GetAllAssetViewContextMenuExtenders();
	}
};

//////////////////////////////////////////////////////////////////////////
// FLyraContentBrowserExtensions

void FLyraContentBrowserExtensions::InstallHooks()
{
	ContentBrowserExtenderDelegate = FContentBrowserMenuExtender_SelectedAssets::CreateStatic(&FLyraContentBrowserExtensions_Impl::OnExtendContentBrowserAssetSelectionMenu);

	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FLyraContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.Add(ContentBrowserExtenderDelegate);
	ContentBrowserExtenderDelegateHandle = CBMenuExtenderDelegates.Last().GetHandle();
}

void FLyraContentBrowserExtensions::RemoveHooks()
{
	TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates = FLyraContentBrowserExtensions_Impl::GetExtenderDelegates();
	CBMenuExtenderDelegates.RemoveAll([](const FContentBrowserMenuExtender_SelectedAssets& Delegate){ return Delegate.GetHandle() == ContentBrowserExtenderDelegateHandle; });
}

//////////////////////////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
