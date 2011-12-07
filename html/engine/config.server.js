/* **************** SERVER/RTS configuration ********** */

// server
var _server = "";

// protocol
var _protocol = "";

// access
var _user = "";
var _passwd = "";


/* **************** API data semantics **************** */
// if API data format changes, redefine column indexes here
// const/var names are uppercase as a kind of syntax highlighting ;]

var _APIDATA =
        {
        TARGETS: { ID:0, NAME:1 },
        FINISHED: { ID:0, TargetID:1, NAME:2, FROM:4, TO:5 }
        };


/* ********************* API URLs ********************* */

/**
* Table used to translate UI request to API url
*/
var _APIURL = {
                /* apiID : URL string */
                "targets":"/targets/api",            // all targets
                "targetsbylabel":"/api/tbylabel?l=#LID#",       // targets by label
                "labellist":"/api/labellist",             // list labels
                "finished":"/nights/#DATE#/api",     // finished targets
                "finbylabel":"/api/tbylabel/#DATE#?l=#LID#",       // targets by label
                "images":"/observations/#ID#/api"   // images by target ID
                                                    // for fullres image add /jpeg/ as prefix and append ?chan=X  for channel X=0-4 X=-1~mosaic
                };

/**
* Function to translate apiID to full URL
*/
function makeURL(apiID)
    {
 //   return(_protocol+_user+':'+_passwd+'@'+_server+_APIURL[apiID]);
    return(_APIURL[apiID]);
    }
