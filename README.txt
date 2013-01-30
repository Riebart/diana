= Getting Started =

== Python Scripts ==
Head over to http://pypy.org/download.html and grab the 2.0b1 package for your
OS of choice. It is very strongly recommended that you do not try to run the
Python scripts with anything other than PyPy for performance reasons (that is,
two orders of magnitude of performance reasons). Once you have PyPy, you're
ready to continue.

The only ones you really need to worry about as a tester are unisim.py and
objectsim.py, with an optional glance at clienttestor.py.

* unisim.py is the universe and underlying physics simulation. It listens on TCP
port 5505 by default. You can start it up by simply running "pypy unisim.py".
** The universe will begin to spit out lines of information, one per second. The
periodic lines it spits out are of the form [[<time that the universe saw pass
last physics tick>, <time that it actually took to perform the last physics
tick>], <time that the last vis-data broadcast took>]

* objectsim.py is the object simulator and is what simulates and runs all of the
ships connected to a universe. It listens on TCP port 5506 by default, and by
default tries to connect to a universe on TCP port 5505 on localhost.

== Client ==
The client is written in C# on .NET and the included files form a Visual Studio
2012 solution. You should be able to head over to
http://www.mono-project.com/Main_Page and grab the version for your OS of
choice. If you're installing from the repos on a Linux machine, make sure you
get the forms package too otherwise it'll error out when it tries to load the
System.Windows.Forms library.

The client's functionality will only be visible if you have both a universe and
an object sim running somewhere you can get to.
