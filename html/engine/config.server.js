/* **************** SERVER/RTS configuration ********** */

// server
var _server = "/";

// protocol
var _protocol = "";

// access
var _user = "user";
var _passwd = "password";


/* **************** API data semantics **************** */
// if API data format changes, redefine column indexes here
// const/var names are uppercase as a kind of syntax highlighting ;]

var _APIDATA =
        {
        TARGETS: { ID:0, NAME:1 },
        FINISHED: { ID:0, TargetID:1, NAME:2, FROM:3, TO:4 }
        };


/* ********************* API URLs ********************* */

/**
* Table used to translate UI request to API url
*/
var _APIURL = {
                /* apiID : URL string */
                "targets":"/targets/api",
                "finished":"/nights/#DATE#/api"
                };

/**
* Function to translate apiID to full URL
*/
function makeURL(apiID)
    {
    return(_protocol+_user+':'+_passwd+'@'+_server+_APIURL[apiID]);
    }
