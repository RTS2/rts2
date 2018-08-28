#!/usr/bin/python

from  xml.etree import ElementTree
import requests
import numpy
from astropy import units as u
from astropy.coordinates import Angle
import sys
import rts2
import urllib
import argparse
rts2.createProxy(url="http://localhost:8889")


def build_url(search_term="M39"):
    urlsafe = urllib.quote_plus(search_term)
    base_url = "http://simbad.u-strasbg.fr/simbad/sim-id?"
    query =  "Ident={}&output.format=votable".format(urlsafe) 
    
    url = base_url+query
    return url


def get_data(st):
    url = build_url(st)
    print("URL is {}".format(url))
    data = requests.get(url)
    doc = ElementTree.fromstring(data.text)
    for ele in doc:
        if "INFO" in ele.tag:
            if ele.attrib["name"] == "Error":
                print("There was an error:")
                print("{}".format(ele.attrib["value"]))
                if 'y' not in raw_input("Continue y/n\n").lower():
                    sys.exit()
    return doc
        
def sort_data(doc):
    prefix = '{http://www.ivoa.net/xml/VOTable/v1.2}'
    fields = []
    vals = []

    for ele in doc[1][0]:
        if ele.tag == prefix+"FIELD":
            fields.append(ele.attrib['name'])


    for ele in doc[1][0][-1][0][0]:
        vals.append(ele.text)

    mydict = dict(zip(fields, vals))
    for key, val in mydict.iteritems():
        if val is not "\n" and val is not None:
            print key, val
    
    return mydict

def search(search_term):
    votable = get_data(search_term)
    target = sort_data(votable)
    target['ra'] = Angle(target['RA_d'], u.deg)
    target['dec'] = Angle(target['DEC_d'], u.deg)
    tar_id = rts2.target.create( target["TYPED_ID"], target['ra'].deg, target['dec'].deg)
    print("Adding target {} at {} {} target id is {}".format( target["TYPED_ID"], target["ra"], target["dec"], tar_id ) )
    return target


def create(rastr, decstr, name ):
    ra = Angle( rastr, unit=u.hour )
    dec = Angle( decstr, unit=u.deg )
    
    tar_id = rts2.target.create( name, ra.deg, dec.deg)
    print("Adding Target {} at {} {} with target id {}".format(name, ra, dec, tar_id))


class create_parser:
    args = None

    def __call__(self, arg):
        if self.args == None:
            self.args = []

        self.args.append(arg)


    

def main():
    cp = create_parser() # negative decs wreak havoc
    parser = argparse.ArgumentParser(description='Add new targets to the RTS2 database')
    parser.add_argument("--search", nargs=1, dest="search_term", help="Search term for Simbad database.")
    parser.add_argument("--create", nargs=3, dest="obj", action="store", type=cp, help="Create a new target with ra, dec and name eg --create 20:42:22 +38:00:00 my_name_for_this_target. If the Declination is negative put the argument in quotes and add a leading space like \" -09:21:14.9\" ")

    args = parser.parse_args()

    if args.search_term:
        search( args.search_term )

    elif args.obj:
        
        create( *cp.args )


        
main()

