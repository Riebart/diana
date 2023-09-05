#!/usr/bin/env python
import urllib2
import re

units = [ 'km', 'kg', '"', 'd', "'", 'K', 'yr', 'gm/s', 'W/m^2', 'km/s', 'm/s^2']

def parse_object_data(raw):
    # In general, it looks like values are termainted by either a unit, or a space... Maybe yes?
    lines = raw.split("\n")
    # Look for asterisk lines
    p = re.compile("^\*\**$")
    end_line = None
    for i in range(1, len(lines)):
        if p.match(lines[i]):
            end_line = i
            break
    lines = lines[:end_line]

    #

def get_object(id, start, end, centre='500@0', make_ephem='YES', table_type='VECTORS', step='1 d', units='KM-S', vect_table='2', ref_plane='ECLIPTIC', ref_system='J2000', vect_corr='NONE', vec_labels='YES', csv_format='YES', obj_data='YES'):
    url = "http://ssd.jpl.nasa.gov/horizons_batch.cgi?batch=1&" + \
    "COMMAND='%s'&" % id + \
    "CENTER='%s'&" % centre + \
    "MAKE_EPHEM='%s'&" % make_ephem + \
    "TABLE_TYPE='%s'&" % table_type + \
    "START_TIME='%s'&" % start + \
    "STOP_TIME='%s'&" % end + \
    "STEP_SIZE='%s'&" % step + \
    "OUT_UNITS='%s'&" % units + \
    "VECT_TABLE='%s'&" % vect_table + \
    "REF_PLANE='%s'&" % ref_plane + \
    "REF_SYSTEM='%s'&" % ref_system + \
    "VECT_CORR='%s'&" % vect_corr + \
    "VEC_LABELS='%s'&" % vec_labels + \
    "CSV_FORMAT='%s'&" % csv_format + \
    "OBJ_DATA='%s'" % obj_data
    url = urllib2.quote(url, safe="%/:=&?~#+!$,;'@()*[]")
    raw = urllib2.urlopen(url).read()
    if csv_format == "YES":
        parse_object_data(raw)
#    print(raw)

get_object(10, "2015-12-31 00:00", "2016-01-01 00:00")
