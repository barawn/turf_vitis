# TURF Vitis instructions

Version control with Vitis doesn't work.

Don't bother trying to search Google, or watch Youtube, or god knows
what else. It doesn't work. Vitis is based on Eclipse, which has nice
Git integration, so it always **seems** like it should work.

But it does not. Many of Xilinx's internal libraries have buried
absolute paths in the build process, and I know libmetal specifically
also has issues. Of course many of Xilinx's makefiles also use
non-portable stuff so Windows/Linux will have issues as well.

So we work around this by creating the platform project locally inside
a workspace that's internal to this repository. Think of it like
creating a Vivado project file, I guess.

Important: Check out this repository using Git command line tools.
Do not use Eclipse's git integration to check it out.

# Build platform project

From Vitis, create a new workspace in the repository directory, in
a subdirectory named "workspace."

File->New->Platform Project. Use "pueo_turf" as the platform project name.

Now in the next screen, do "Browse" and go and find the "pueo_turf.xsa" file
in the "hw" in this repository. Click through things and finish. Right click
on pueo_turf and Build. It should work. One hopes.

# Import turf startup.

File->Import. Import from Git. Local repository. Navigate to the repository,
and click Next. Import existing Eclipse projects.

Select both turf_startup and turf_startup_system.

# How this works

The only reason this works is because after creating the application,
I edited the sprj (system project) and prj (project) files to remove
the absolute paths and instead have them point to paths relative
to the internal workspace.

IF YOU EVEN VIEW EITHER OF THOSE (turf_startup.prj or turf_startup_system.sprj)
IN VITIS, YOU NEED TO AGAIN EDIT THOSE FILES AND GET RID OF THE ABSOLUTE
PATHS BEFORE CHECKING BACK IN.

The key item to look for in the XML is the "platform" entry. It should be
a relative path to pueo_turf.xpfm instead of the relative path.
