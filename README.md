# StoutREU
Visual Studio Project source for 2018 robotics REU at UW-Stout

# Drivers for Jaco2 Arm on Windows 10
* install Kinovo SDK for JACO2 arm
* disable secure boot in BIOS menu
* run cmd as admin
* bcdedit.exe -set loadoptions DISABLE_INTEGRITY_CHECKS
* bcdedit.exe -set TESTSIGNING ON
* restart
* run device manager
* install driver for the arm device from the location of SDK

# Microsoft Speech Sdk for Speech Recognition
* Download Microsoft Speech SDK at the link: https://www.microsoft.com/en-us/download/details.aspx?id=43662
* Download the version that corresponds with your OS (x86) - 32bit (x64) - 64bit
* Rebuild the Visual Studio project
