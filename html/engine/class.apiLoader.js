/**
* ******* RTS UI LOADER CLASS *******
* Loads (JSON) data from API and optionaly parses by eval()
* @parameter target {DOM node} target DIV for processing messages
* @parameter apiID {string} api function name/ID, translated by API array into URL
*
* RESULTS:
* this.rq ... XMLHttpRequest
* (this.rq.responsetext ... data given by page)
* this.data ... parsed object if this.json enabled
*
* @constructor
*/
function apiLoad(slot)
{

// target div
this.slot = slot;

// URL to load
this.url = '';

// XMLHttpRequest
this.rq = null;

// proxy - if defined, sends URL with this prefix
//this.proxy = "proxy.php?url=";

// if on - response is parsed by eval()
this.json = false;

// object parsed from response by eval()
this.data = null;


/**
* Start the transfer
*/ 
this.load = function(url)
    {
    this.url = url;
    
    // load message
    this.slot.message('<img src="gfx/ajax-loader.gif" border="0" width="32" height="32" alt="X"><br><br>loading');
    
    // URL juggling
    var url = this.url;
    if(this.proxy) { url = this.proxy+this.url; }

    // create request
    this.rq=new XMLHttpRequest();
    
    // exec
    try
    {
       this.rq.open("GET",url,false);
    } catch(err) {
       status('JSON get error on ' + url +err.toString());
       return false;
    }
    status("Loading "+url);
    this.rq.send();   

    if (this.rq.status != 200)  { this.slot.message("HTTP Error / server status: "+xhReq.status);return false;}
    this.slot.message('Processing ...');

    
    try {
        if(this.json) {
                    if(window.JSON)
                        { this.data = window.JSON.parse(this.rq.responseText); }
                        else
                        { this.data = eval('('+this.rq.responseText+')'); }
                    }
        } catch(err) {
            status('JSON parser error: '+err.toString());
            this.slot.message('<b>ERROR: Malformed JSON data.</b><br>URL: '+url+'<br><a href="javascript:'+this.slot.name+'.showRAW();">[view raw]</a>');
            return false;
            }
    return true;    
    }       
    
} // apiLoader class end
