WHAT IS THIS?
=============

Linux Kernel source code for the device bq Aquaris E4.5

The sources for the Ubuntu Edition can be found on the branch
aquaris-E4.5-ubuntu-rtm. The README.md there covers building, etc for
Ubuntu.

BUILD INSTRUCTIONS?
===================

Specific sources are separated by branches and each version is tagged with it's corresponding number. First, you should
clone the project:

	$ git clone git@github.com:bq/aquaris-E4.5.git

After it, choose the version you would like to build:

	$ cd aquaris-E4.5/
	$ git checkout 1.2.1_20140721-0600


Finally, build the kernel:

	$ ./makeMtk -t krillin n k

