# StoutREU
Visual Studio Project source for 2018 robotics REU at UW-Stout

# Drivers for Jaco2 Arm on Windows 10
* Download Jaco2 sdk: https://www.kinovarobotics.com/en/knowledge-hub/all-kinova-products
* install Kinovo SDK for JACO2 arm
* disable secure boot in BIOS menu
* run cmd as admin
* bcdedit.exe -set loadoptions DISABLE_INTEGRITY_CHECKS
* bcdedit.exe -set TESTSIGNING ON
* restart
* run device manager
* install driver for the arm device from the location of SDK

# Microsoft Kinect V2 SDK
* Download the kinect V2 SDK: https://www.microsoft.com/en-us/download/details.aspx?id=44561

# Microsoft Speech Sdk for Speech Recognition
* Download Microsoft Speech SDK at the link: https://www.microsoft.com/en-us/download/details.aspx?id=43662
* Download the version that corresponds with your OS (x86) - 32bit (x64) - 64bit (recommended x86)
* Download the Speech runtime and Language pack
* Rebuild the Visual Studio project

# Open CV
* Download open cv source files (x86): https://github.com/opencv/opencv
* Download extra modules (contrib modules includes aruco library): https://github.com/opencv/opencv_contrib
* Use cmake-gui https://cmake.org/download/
* Configure
* Set the extra modules path to the "modules" folder in the downloaded contrib folder
* Generate
* Go to the generated visual studio project
* Build All_build in x86 debug
* build All_build in x86 release
* Build Install in x86 reslease
* build Install in x86 debug

# Set paths in Visual Studio
* Set paths for include directories, additional dependencies and lib files (use debug version of opencv lib files)
