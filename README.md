# MLTest2

## Guide to Building off the Git Repo:
---------------------------------------------
1. Build UE4.26 from source: https://github.com/EpicGames/UnrealEngine/archive/refs/tags/4.26.2-release.zip
-----
1.a. Download the release zip and unzip the folder in C:\UE4.26Source directory or a similar directory. (It must be a root folder, otherwise the directory lines get too long.)
1.b. Run the Setup.bat file.
1.c. Run the GenerateProjectFiles.bat file.
1.d. Open the UE4.sln file in Visual Studio, set the build options in the top left to Development Editor and Win64. Right click the UE4 entry in the tree on the right of the screen, and click build. Usually takes 45 minutes.
-----

2. Run

$ git clone https://github.com/njd5368/arvr


in the directory you want to save the project to.

3. Right click the MLTest2.uproject file in the arvr directory that was created by the clone command, and select Generate Visual Studio project files

4. Open the MLTest2.sln file that generated in Visual Studio, set the build options in the top left to Development Editor and Win64. Right click the MLTest2 entry in the tree on the right of the screen, and click build.

5. Keep Visual Studio open from the last step, and instead, set the build options in the top left to Development Server and Win64. Right click the MLTest2 entry in the tree to the right of the screen, and click build.

Various Errors and Fixes:
-----------------------------------------
If the lighting fails to build with Lightmass needs rebuilding or something to that extent, open the .sln for the project and build the UnrealLightmass module found in the Programs folder. 
