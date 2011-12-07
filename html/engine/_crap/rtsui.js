
/* ================================ STATUS ===================================== */
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

/* ============================= GENERAL CLASSES ================================= */

/**
* ******* RTS UI SLOT CLASS *******
* Slot is a block (DIV) to build the UI
* Slot function .exec() is called by linx/buttons to change the slot content
* @parameter targetID {string} target DIV for processing messages and output
*
*
* @constructor
*/
function slot(targetID)
{

// processing module
this.module = null;

// loader class (apiLoader)
this.loader = null;

// parsed data (raw server response available in this.loader.rq.responseText)
this.data = null;

// target div
this.target = document.getElementById(targetID);
if(!this.target) {throw('Target element not found: '+targetID);}



/**
* Function to change slot without reloading data
* @parameter targetID {string} target DIV for processing messages
* @parameter apiID {string} api function name/ID, translated by API array into URL
*/
this.exec = function(module,command)
    {
    // if another module is used to vizualize output ... create the instance
    if((!this.module)||(this.module.constructor!=module))
        {
        this.module = new window[module](this);
        if(!this.module) {throw('Unable to create module instance: '+module);}
        }
    this.module.exec(command);        
    }


    
/**
* Function to (re)load data of the slot and display 
* @parameter apiID {string} api function name/ID, translated by API array into URL
* @parameter module {string} class name of processing module
* @parameter command {JSON} command object (parameters definition for action)
*/
this.reload = function(apiID,module,command)
    {
    // store parameters
    this.command = eval('('+command+')');
    
    // translate api ID to URL
    var url=makeURL(apiID);

    // load new data
    this.loader = new apiLoad(this);
    this.loader.json = true;
    this.loader.load(url);
    this.data = this.loader.data; // local reference

    // excecute command
    this.exec(module,command);    
    }


/**
* Sets slot (div) content
* @parameter html {string} 
*/
this.setContent = function(html)
    {
    // show slot
    this.target.style.display = "block";
    // set content
    this.target.innerHTML = html;
    }

/**
* Sets slot GUI message
* @parameter msg {string} 
*/
this.message = function(msg)
    {
    // show slot
    this.target.style.display = "block";
    // set content
    this.target.innerHTML = msg;
    }

    
    
}

/* ================================================================================= */

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
this.proxy = "proxy.php?url=";

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
    this.slot.message('Loading ...');
    
    // URL juggling
    var url = this.url;
    if(this.proxy) { url = this.proxy+this.url; }

    // create request
    this.rq=new XMLHttpRequest();
    
    // exec
    this.rq.open("GET",url,false);
    this.rq.send();   

    if (this.rq.status != 200)  { this.slot.message("HTTP Error / server status: "+xhReq.status);return;}
    this.slot.message('Processing ...');

    if(this.json)
        {this.data = eval('('+this.rq.responseText+')');}
    }  
    
    
} // rtsUI class end


