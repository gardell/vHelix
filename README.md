vHelix - Free-form DNA-nanostructure design
======
http://www.vhelix.net/

An Autodesk Maya 2011 plugin for modelling DNA based structures.

The plugin is available for the 64-bit versions of Autodesk Maya 2011 for Windows, Linux and Mac OSX. The plugin is currently not available for older versions of Maya and not the 32-bit versions. The plugin might work with newer versions of Maya but this has not been verified.

Visit http://www.vhelix.net/ for ready to use releases. The releases supplied over there are not updated as often as the ones hosted here on Github. Notice that the versions here can at some times be broken and non-usable. However, new features and bug fixes will always be released here first.

To install the required files from here, instead of using the prepacked files at http://www.vhelix.net/:
* Either do a clone from the git repository, or download the files directly from the web site
* For Linux or Mac OS X builds, navigate to the 'Debug' folder. For Windows builds, navigate to the 'Release' folder. Notice that Windows user can use the Debug builds if they have Visual Studio 2010 installed.
* Windows users: Copy the vHelix.mll file. Mac OS X users: Copy the vHelix.bundle. Linux users: Copy the libvHelix.so.
* Place the file under your Maya plug-ins folder. Windows: typically 'C:\Program Files\Autodesk\Maya2011\bin\plug-ins'. Mac OS X: typically: '/Applications/Autodesk/maya2011/Maya.app/Contents/MacOS/plug-ins/'. Linux: typically '/usr/autodesk/maya/bin/plug-ins'
* Navigate to the 'res' folder and copy or download the 'BackboneArrow.ma'. Place the file in the 'maya/projects/default/scenes/' usually found in your home directory. Windows: typically found in your 'My Documents' or 'Documents' folder. Mac OS X users: typically '/Users/<username>/Documents/maya/projects/default/scenes' where <username> is your username. Linux users: typically '/home/<username>/maya/projects/default/scenes' where <username> is your username.

Notice: If you are not able to install plug-ins in the Autodesk Maya plug-ins directory due to lack of administration privileges or other, it is still possible to run the plugin.
* Place the plugin file in any directory in which you have access.
* Open Autodesk Maya 2011
* Open the Script Editor by clicking the icon in the further right bottom corner or by clicking 'Window' > 'General Editors' > 'Script Editor'
* In the box labeled 'MEL' enter the following code:

Windows:
loadPlugin "C:/<path-to-your-vHelix-plugin-mll-file>/vHelix-Project.mll"

Mac oS X:
loadPlugin "/Users/<path-to-your-vHelix-plugin-mll-file>/vHelix.bundle"

Linux:
loadPlugin "/home/<path-to-your-vHelix-plugin-mll-file>/libvHelix.so"

* Notice that the code entered can be made into an icon for easier loading in the future. Enter the code and then click 'File' > 'Save Script to Shelf...' in the 'Script Editor' window.