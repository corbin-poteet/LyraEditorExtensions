# LyraEditorExtensions
 Sample editor plugin for extending Unreal Engine 5's content browser and level editor. Made specifically for the Lyra framework, but *should* work with any Unreal project. Heavily based on Epic's implementation of the Paper2D plugin. Comes with example texture unpacker.

## Installation
1. Unzip or clone into your project's Plugins folder.
2. Enable the plugin in-engine or in the .uproject file.

## Example Features
### Texture Unpacker
![image](https://github.com/corbin-poteet/LyraEditorExtensions/assets/4257207/bab42f00-b3e7-4766-ae03-d24337cc0459)
![image](https://github.com/corbin-poteet/LyraEditorExtensions/assets/4257207/32c84ac3-8225-4f92-ae82-9dc2cf2e0bfe)
#### Features
* Extracts texture color channels into their own greyscale textures.
* Smart unpacking- won't unpack pure black or white channels, or unpack from textures with Grayscale or Normalmap compression settings.
* Automatic texture settings- new unpacked textures automatically use the original texture's Mip Gen Settings and Texture Group, use Grayscale compression settings, and compress without the alpha channel.
