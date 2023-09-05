This is going to be pretty basic.

Hit up the wiki at https://www.riebrat.ca/diana for almost all of the information.

= Using the source =
Compile the Universe Sim (unisim), either with Visual Studio 2015 on Windows,
or with the included Makefile on Unix platforms. It uses cross-platform code
everywhere possible, to unify the code-base and reduce the amount of #ifdef
that is required to support multiple platforms. Much of this is accomplished
through the C++11 concurrency features, which removes the platform-specific
dependencies in that area.

The unisim listens on port 5505 by default.

Python 2.7 is required to run the Object Sim (osim), which is driven by
objectim.py. Other test kits include clienttestor.py which connects directly
to the unisim, and testory.py which connects to the osim. The former includes
tests that test specific universe functionality. The latter includes higher
level tests, such as ship functionality and full-stack tests like the homing
missile test.

The osim listens on port 5506 by default.
 
The 2D client (Diana2DClient) is written in .NET, and uses Windows forms for
rendering a view down the Z axis. It should run under mono, see the notes in
the project's folder for running it under Mono. This should compile under
Visual Studio 2015.

The client built on Unreal Engine 4 (UEClient) is located in the EventTest
folder, and requires the UDK 4.10 or higher to work with at this time.

== Running Things ==
* Start the unisim.
* Start the osim.
* Start a client.
* Run a python script to generate some objects/scenario in the universe.

= PyPy =

PyPy doesn't ship, and sometimes won't pick up the system packages.

Diana requires bson, and two dependencies that requires; pytz and six.

- Grab the pure BSON library from https://github.com/py-bson/bson
  > Extract the bson directory, and pitch the rest.
- Grab a pytz source package from https://pypi.python.org/pypi/pytz/#downloads
  > Extract the pytz folder, and pitch the rest.
- Grab a six source package from https://pypi.python.org/pypi/six#downloads
  > Extract the six.py file, and pitch the rest.

Put those two directories and the source file in the Diana root, and away we go!
