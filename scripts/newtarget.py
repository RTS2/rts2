from  xml.etree import ElementTree
import requests
import numpy
from astropy import units as u
from astropy.coordinates import Angle
import sys
import rts2
import urllib
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

def newtarget(search_term):
    votable = get_data(search_term)
    target = sort_data(votable)
    target['ra'] = Angle(target['RA_d'], u.deg)
    target['dec'] = Angle(target['DEC_d'], u.deg)
    tar_id = rts2.target.create( target["TYPED_ID"], target['ra'].deg, target['dec'].deg)
    print("Adding target {} at {} {} target id is {}".format( target["TYPED_ID"], target["ra"], target["dec"], tar_id ) )
    return target




search_term = sys.argv[1]
newtarget(search_term)
