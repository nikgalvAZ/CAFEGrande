General runtime dependencies for all platforms:
- WX widgets libraries (requires separate installation, see below)
- PVCAM Runtime libraries (downloadable from Photometrics website)
- Photometrics helper libraries "pvcam_helper_color" and "pvcam_helper_track"
    - These are optional but included with the project


On Windows:
===========

Run-time dependences:
---------------------

- PVCAM Runtime libraries 3.9.9.1 or newer
- WX widgets 3.1.5 (base, core and propgrid libraries)
- C++ Runtime Redistributable for Visual Studio 2015/2017/2019

Build process:
------------------

If this project was installed by an installer or otherwise placed into a
restricted location, you may need to copy the project folder into Documents,
Desktop or other folder accessible by the user. Otherwise, Administrator
rights may be needed to build the project.

The application is statically linked with LibTIFF libraries, dynamically
with WX widgets and PVCAM libraries. LibTIFF libraries are included in the
project folder. WX widgets need to be downloaded and configured manually.
This project is part of PVCAM SDK that provides the required PVCAM headers
and development libraries.

Please download and setup WX widgets as described here:
- https://docs.wxwidgets.org/3.1.5/plat_msw_binaries.html

Specifically for this project:
- Go to "www.wxwidgets.org/downloads/", select "Download Windows Binaries"
  and download following archives:
    - Header Files (wxWidgets-3.1.5-headers.7z)
    - 32-Bit (x86) Development Files (wxMSW-3.1.5_vc14x_Dev.7z)
    - 64-Bit (x86_64) Development Files (wxMSW-3.1.5_vc14x_x64_Dev.7z)
- Extract the archives to a local folder, this guide assumes that
  "C:\wxWidgets\3.1.5_vc14x" will be used
    - Some files may get overwritten during the extraction of the x86 and
      x86_64 archives
    - After extraction, the folder structure should be as follows:
    - C:\wxWidgets\3.1.5_vc14x
        - build
        - include
        - lib
        - wxwidgets.props
- On Windows, use "Edit the system environment variables" control panel
  and add a new System variable:
    - Name: WXWIN
    - Value: C:\wxWidgets\3.1.5_vc14x

After that, use the PVCamTest.sln to open the project in Visual Studio and use
Batch Build to verify that all configurations can be built successfully. This
guide was created for Visual Studio 2019 but Visual Studio versions 2015, 2017
and newer will likely work too.

You may need to select the "PVCamTest" project as a "Startup Project" in order
to build and launch the GUI part of the project.


On Linux:
=========

This project is part of PVCAM for Linux package. The "pvcam" runtime
needs to be installed in order to build and run this project.

Run-time dependences:
---------------------

On Ubuntu, use "apt" and install the following packages:
- libwxgtk3.0-0v5      (GTK+ 2 version, on Ubuntu 16.04 and 18.04)
- libwxgtk3.0-gtk3-0v5 (GTK+ 3 version, on Ubuntu 18.04 and 20.04)
- libtiff5

Build process:
------------------

The application is dynamically linked with WX widgets and libtiff libraries.

On Ubuntu, use "apt" and install the following packages:
- libwxgtk3.0-dev      (GTK+ 2 version, on Ubuntu 16.04 and 18.04)
- libwxgtk3.0-gtk3-dev (GTK+ 3 version, on Ubuntu 18.04 and 20.04)
- libtiff5-dev

If this project was installed by an installer or otherwise placed into a
restricted location, you may need to copy the project folder into the user's
home folder or other accessible location before the build. For example:
    $ cp -r /opt/pvcam/sdk/examples/PVCamTest ~/

The PVCamTest project consists of two sub-projects, a "PVCamTest" GUI
application and a "PVCamTestCli" application. To build each application,
navigate to the respective sub folder and "make" the project.

For example, to build the main GUI application, navigate to the "PVCamTest"
sub folder and type in the terminal:
    $ WX_TOOLKIT=gtk3 make

The WX_TOOLKIT environment variable is required for Ubuntu 18.04 and newer and
when it's desired to build the GTK3 version of the GUI. When this variable is
omitted, the GTK2 version of the GUI will be built.
