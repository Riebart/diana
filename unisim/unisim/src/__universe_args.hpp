double opt_beam_energy_cutoff = parser.get_basic_option(
            "",
            "--beam-energy-cutoff",
            1e-10,
            "On initialization, a beam has a maximum distance that is calculated from it's spread values, energy, and this cutoff. The maximum distance, D, is the amount of distance travelled, such that the wavefront at D distance from the source has less than this amount of energy (Joules) per square metre of wavefront area. Raising this value will expire beams sooner, and this may improve performance if large number of beams are in use. This value is derived from commodity wireless transceivers that operate at -70dBmW. ", false).result.option_value;
params.beam_energy_cutoff = opt_beam_energy_cutoff;
double opt_collision_energy_cutoff = parser.get_basic_option(
            "",
            "--collision-energy-cutoff",
            1e-9,
            "Collisions (physical or beam) that result in a transfer of energy below this amount are ignored. This helps to ensure that spurious collisions (stiction) are gracefully handled. ", false).result.option_value;
params.collision_energy_cutoff = opt_collision_energy_cutoff;
double opt_gravitational_constant = parser.get_basic_option(
            "",
            "--gravitational-constant",
            6.67384e-11,
            "Universal gravitational constant. ", false).result.option_value;
params.gravitational_constant = opt_gravitational_constant;
double opt_gravity_magnitude_cutoff = parser.get_basic_option(
            "",
            "--gravity-magnitude-cutoff",
            0.01,
            "Objects that would produce a gravitational acceleration below this amount at their bounding radius are not considered attractors in the universe. This is used to optimize the selection of objects that are considered attractors for practical purposes, to reduce the O(N^2) nature of gravitational calculations. ", false).result.option_value;
params.gravity_magnitude_cutoff = opt_gravity_magnitude_cutoff;
double opt_health_damage_threshold = parser.get_basic_option(
            "",
            "--health-damage-threshold",
            0.1,
            "During a collision, non-smart objects make take damage equal to one point of health per Joule of collision energy that exceeds this proportion of the object's current health. ", false).result.option_value;
params.health_damage_threshold = opt_health_damage_threshold;
double opt_health_mass_scale = parser.get_basic_option(
            "",
            "--health-mass-scale",
            1e6,
            "Non-smart physics objects are assigned a number of hit points that is their mass multiplied by this value. ", false).result.option_value;
params.health_mass_scale = opt_health_mass_scale;
double opt_max_physics_frametime = parser.get_basic_option(
            "",
            "--max-physics-frametime",
            0.002,
            "Maximum unscaled simulated time that is allowed to pass in a single tick. If the previous physics tick took more wall-clock time than this, the time delta for the next tick is reduced to this amount. If physics ticks routinely take more wall clock time than this amount, the clamping of simulated time will result in a perceived slowdown of the simulation to slower than realtime. Setting this value too high may result in a physics simulation that is too coarse. Setting this value too low may result in unnecessary apparent slowdown of the game to achieve a time step that may be finer than necessary. ", false).result.option_value;
params.max_physics_frametime = opt_max_physics_frametime;
double opt_max_simultaneous_collision_rounds = parser.get_basic_option(
            "",
            "--max-simultaneous-collision-rounds",
            100,
            "Maximum number of rounds of collision simulation in which simultaneous collisions are considered. This value is only going to come into play with precisely placed objects like done in code. True simultaneous collisions are astronomically unlikely to occur in a real simulation scenario. ", false).result.option_value;
params.max_simultaneous_collision_rounds = opt_max_simultaneous_collision_rounds;
double opt_min_physics_frametime = parser.get_basic_option(
            "",
            "--min-physics-frametime",
            0.002,
            "Print verbose information, such as collision rounds per tick, and when a collision happens, and which objects were involved. bool verbose_logging; Minimum unscaled simulated time that is allowed to pass in a single tick. If the previous phsyics tick took less wall-clock time than this, the time delta for the next tick is increased up to this amount. Setting this value too low will result in increased CPU use on the physics server to simulate at a time step that is finer than necessary. Setting this value too high may result in too coarse of a physics time step. ", false).result.option_value;
params.min_physics_frametime = opt_min_physics_frametime;
double opt_min_vis_frametime = parser.get_basic_option(
            "",
            "--min-vis-frametime",
            0.1,
            "The minimum wall-clock time that will pass between subsequent rounds of vis-data transmission. Setting this value too low will result in vis data transmission beginning to interfere with the simulation's ability to effectively compute physics ticks at a sufficient rate. It is recommended that clients perform local interpolation to provide smooth apparent motion in between the vis data update frames. First-order interpolation should be considered minimal, with second-order interpolation preferable. double min_vis_frametime; ! Rate (1.0 = real time) at which to simulate the world. Useful for speeding up orbital mechanics. double rate; ! Total time elapsed in the game world double total_time; ! Last persitent environmental effect time. Total simulation time that the last event was ! triggered for the last environment effect (radiation, etc...) double last_effect_time; ! The minimum time to spend on a physics frame, this can be used to keep CPU usage down or to smooth out ticks. double min_frametime; ! The maximum allowed time to elapse in game per tick. This prevents physics ticks from getting too coarse. double max_frametime; ! The time spent calculating the physics on the last physics tick. double phys_frametime; ! The time elapsed calcualting physics and sleeping (if that happened) on the last physics tick. double wall_frametime; ! The time elapsed in the game world on the last physics tick. double game_frametime; ! The minimum time to spend between VISDATA blasts. ", false).result.option_value;
params.min_vis_frametime = opt_min_vis_frametime;
bool opt_permit_spin_sleep = parser.get_flag_option(
            "",
            "--permit-spin-sleep",
            "Permit spin sleeping to slow realtime simulations down to real time. ", false).result.option_value;
params.permit_spin_sleep = opt_permit_spin_sleep;
double opt_radiation_energy_cutoff = parser.get_basic_option(
            "",
            "--radiation-energy-cutoff",
            1.5e4,
            "This value is used to calculate the safe distance of a radiation source, that is the distance from the source at which other objects begin to incur radiation collision events (and damage). Raising this value will reduce the damage caused by radiation sources. The unts of this value is Watts per square metre. This value is derived from the black-body radiative power of steel at it's melting point. Steel absorbing this amount of radiative power. See: http://nssdc.gsfc.nasa.gov/planetary/factsheet/mercuryfact.html See: https://en.wikipedia.org/wiki/Mercury_(planet)#Surface_conditions_and_exosphere To compare, Sol outputs 61.7MW/m^2 at it's surface. ", false).result.option_value;
params.radiation_energy_cutoff = opt_radiation_energy_cutoff;
bool opt_realtime_physics = parser.get_flag_option(
            "",
            "--realtime-physics",
            "When set, the simulation will sleep as appropriate to try to match the amount of simulated time to the amount of elapsed wall-clock time. When using this option, careful and informed selection of minimum and maximum physics tick time limits should be used to ensure a smooth and sufficiently fine simulation without forcing overly fine simulation at the expense of a slowdown. ", false).result.option_value;
params.realtime_physics = opt_realtime_physics;
double opt_simulation_rate = parser.get_basic_option(
            "",
            "--simulation-rate",
            1.0,
            "When paired with realtime_physics, this controls a simulation rate relative to realtime. Values <1.0 result in simulations that are slower than real time, and values >1.0 result in simulations faster than realtime. ", false).result.option_value;
params.simulation_rate = opt_simulation_rate;
double opt_spectrum_slush_range = parser.get_basic_option(
            "",
            "--spectrum-slush-range",
            0.01,
            "Spectrum power levels are randomly adjusted by this proportion upon receipt by the universe to provide a statistical guarantee that two objects don't have identical signature spectra. If teh power level is below this amount, then it is set to a random value between 0 and this value, inclusive. ", false).result.option_value;
params.spectrum_slush_range = opt_spectrum_slush_range;
double opt_speed_of_light = parser.get_basic_option(
            "",
            "--speed-of-light",
            299792458l,
            "The speed of light in this universe. The defualt speed for beams, and the speed at which relativistic effects do/would take effect. ", false).result.option_value;
params.speed_of_light = opt_speed_of_light;
bool opt_verbose_logging = parser.get_flag_option(
            "",
            "--verbose-logging",
            "Print verbose information, such as collision rounds per tick, and when a collision happens, and which objects were involved. ", false).result.option_value;
params.verbose_logging = opt_verbose_logging;
double opt_visual_acuity = parser.get_basic_option(
            "",
            "--visual-acuity",
            4e-7,
            "The cutoff (in (radius/distance)^2) used to determine whether a given object is sent as a piece of visual data. The square is to prevent unnecessary square roots being performed frequently. The physical units of this tan(radians). This assumes an acuity of approximately 60cm at 1km, which roughly corresponds to a typical human ", false).result.option_value;
params.visual_acuity = opt_visual_acuity;
