
/**
* Status DIV
*/
var statusDiv = null;


/**
* Update status message
*/ 
function status(text)
    {
    if(!statusDiv) { statusDiv = document.getElementById('status'); }
    statusDiv.innerHTML = text;
    }



/**
* ******* RTS UI SLOT MANAGEMENT CLASS *******
* Slot is a block (DIV) to build the UI
* Slot function .exec() is called by linx/buttons to change the slot content
* @parameter targetID {string} target DIV for processing messages and output
*
*
* @constructor
*/
function ui(targetID)
{

// processing module
this.module = null;

// target div
this.target = document.getElementById(targetID);
if(!this.target) {throw('Target element not found: '+targetID);}



/**
* Function to change slot without reloading data
* @parameter targetID {string} target DIV for processing messages
* @parameter apiID {string} api function name/ID, translated by API array into URL
*
*
* @constructor
*/
this.exec = function(module,command)
    {
    // if another module is used to vizualize output ... create the instance
    if(this.module.constructor!=module)
        {
        new window[module](this);
        if(!this.module) {throw('Unable to create module instance: '+module);}
        }
    this.module.exec(command);        
    }


    

    
/**
* Function to (re)load data of the slot and display 
* @parameter apiID {string} api function name/ID, translated by API array into URL
* @parameter module {string} class name of processing module
* @parameter command {string} command for module (module specific)
*
*
* @constructor
*/
this.reload = function(apiID,module,command)
    {
    l = new apiLoad()
    rl = new rtsuiLoad('main','dumper',"http://user:password@host");
    }


}



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
function apiLoad(target,url)
{

// target div
this.target = target;

// URL to load
this.url = url;

// XMLHttpRequest
this.rq = null;

// proxy - if defined, sends URL with this prefix
this.proxy = "proxy.php?url=";

// if on - response is parsed by eval()
this.json = false;

// object parsed from response by eval()
this.data = null;


/**
* Start the transfer
*/ 
this.start = function()
    {
    // load message
    this.target.innerHTML = 'Loading ...';
    
    // URL juggling
    var url = this.url;
    if(this.proxy) { url = this.proxy+this.url; }

    // create request
    this.rq=new XMLHttpRequest();
    // connect handlers
    this.rq.loader = this;
    //this.rq.onreadystatechange = this.onResponse;
    //this.rq.ontimeout = this.onTimeout;
    
    // exec
    this.rq.open("GET",url,true);
    this.rq.send();   
    }


/**
* Take response, handle errors, pass data to module on success
*/    
this.onResponse = function()
    {
    if (this.readyState != 4)  { return; } // not ready yet
    if (this.status != 200)  { this.loader.target.innerHTML="HTTP Error / server status: "+xhReq.status;return;}
    this.loader.target.innerHTML = 'Processing ...';

    if(this.json)
        {this.data = eval('('+this.loader.rq.responseText+')');}
    }  
    
this.onTimeout = function()
    {
    this.target.innerHTML = 'Request timeout.';
    }  
    
} // rtsUI class end


