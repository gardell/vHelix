vHelix - Free-form DNA-nanostructure design
======
http://www.vhelix.net/

An Autodesk Maya plugin for modelling DNA based structures.

The plugin is available for the 64-bit versions of Autodesk Maya 2011, 2012, 2013 and 2014 for Windows, Linux and Mac OS X. The plugin is not available for other versions of Maya or 32-bit versions. The plugin might work with newer versions of Maya but has to be recompiled for these versions.

Notice that the versions hosted on vHelix refer to the ones here and can at some times be broken or non-usable as they are being actively developed on.

To install the required files from here, instead of using the prepacked files at http://www.vhelix.net/:
* Either do a clone from the git repository, or download the files directly from the web site
* For Linux or Mac OS X builds, navigate to the 'Debug' folder. For Windows builds, navigate to the 'Release' folder. Notice that Windows user can use the Debug builds if they have Visual Studio 2013 installed.
* Windows users: Copy the vHelix.mll file. Mac OS X users: Copy the vHelix.bundle. Linux users: Copy the libvHelix.so.
* Place the file under your Maya plug-ins folder. Windows: typically 'C:\Program Files\Autodesk\Maya2011\bin\plug-ins'. Mac OS X: typically: '/Applications/Autodesk/maya2011/Maya.app/Contents/MacOS/plug-ins/'. Linux: typically '/usr/autodesk/maya/bin/plug-ins'

Notice: If you are not able to install plug-ins in the Autodesk Maya plug-ins directory due to lack of administration privileges or other, it is still possible to run the plugin.
* Place the plugin file in any directory in which you have access.
* Open Autodesk Maya
* Open the Script Editor by clicking the icon in the further right bottom corner or by clicking 'Window' > 'General Editors' > 'Script Editor'
* In the box labeled 'MEL' enter the following code:

Windows:
loadPlugin "C:/<path-to-your-vHelix-plugin-mll-file>/vHelix-Project.mll"

Mac oS X:
loadPlugin "/Users/<path-to-your-vHelix-plugin-mll-file>/vHelix.bundle"

Linux:
loadPlugin "/home/<path-to-your-vHelix-plugin-mll-file>/libvHelix.so"

* You can save the command entered into a script for easier loading in the future. Enter the code and then click 'File' > 'Save Script to Shelf...' in the 'Script Editor' window.

* There are many MEL commands available with vHelix not visible in the Helix menu of Maya, visit https://docs.google.com/document/d/19P08qzSYyztWXMV7whR-XninQwg4oBOjDF-qghzpHTo/edit?usp=sharing for a complete documentation.