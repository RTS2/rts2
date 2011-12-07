/**
* ******* RTS UI SLOT CLASS *******
* Slot is:
*      - a block (DIV) to build the UI
*      - container of loaded AJAX data
*      - rendered by module instance [set by .setModule(), controlled by .parameters object]
* Main functions:
*       load(id) ... loads JSON data
*       setModule(name) ... creates instance of rendering module
*       render() ... renders content [by calling <module>.render()]
*       event(id,data) ... receives event 
*       show/hide ... display control
*
* @parameter targetID {string} target DIV for processing messages and output
* @constructor
*/
function slot(targetID)
{

// processing module
this.module = null;

// global slot parameters (shared by modules)
this.parameters = 
    {
    date: null // date to display
    }
this.parameters.date = new Date();

// saved apiID for reload
this.oldApiID = null;

// loader class (apiLoader)
this.loader = null;

// parsed data (raw server response available in this.loader.rq.responseText)
this.data = null;

// name of global scope JS variable containing the slot object
// (used for back-refference from the content hyperlinks)
this.name = targetID;


// target div
this.target = document.getElementById(targetID);
if(!this.target) {throw('Target element not found: '+targetID);}

// slot DIV has closing button
this.closeable = false;

// slot title
this.title = null;


// slot menu - see iconMenu for definition
this.menu =  null;


/**
* Function to display the slot DIV
*/
this.show = function() { this.target.style.display = 'block'; }

/**
* Function to display the slot DIV
*/
this.hide = function() { this.target.style.display = 'none'; }

/**
* Function to change switch rendering
* @parameter module {string} module name
*/
this.setModule = function(module)
    {
    // if another module is used to vizualize output ... create the instance
    if((!this.module)||(this.module.constructor!=module))
        {
        this.module = new window[module](this);
        if(!this.module) {throw('Unable to create module instance: '+module);}
        }
  //  this.module.render();        
    }



/**
* Function to render slot content
*/
this.render = function()
    {
    this.show();
    this.module.render();        
    }


/**
* Function to receive event
* @parameter id {string} event id
* @parameter data {object} data to process
*/
this.event = function(id,data)
    {
    this.module.event(id,data);        
    }


    
/**
* Function to load data of the slot and display 
* @parameter apiID {string} api function name/ID, translated by API array into URL
*/
this.load = function(apiID)
    {
    // translate api ID to URL
    var url=makeURL(apiID);
    
    // parameter processing
    if(this.parameters)
        {
        // param injection into URL - replaces #-strings
        if(typeof(this.parameters.date)!='undefined') { url = url.replace('#DATE#',dateStr(this.parameters.date)); }
        }
    

    // load new data
    this.loader = new apiLoad(this);
    this.loader.json = true;
    var success = this.loader.load(url);
    this.data = this.loader.data; // local reference
    
    // store apiID for reload
    if(success) { this.oldApiID = apiID;} 
    
    return success;
    }

/**
* Reloads slot data using old apiID
*/
this.reload = function()
    {
    if(this.oldApiID) { return this.load(this.oldApiID);}
        else { status('No data - nothing to reload.');} 
    }



/**
* Combo FX for loading and displaying content in slot
* @parameter apiID {string} api function name [see .load()], skips loader if null
* @parameter module {string} class name of processing module
* @parameter parameters {Object} parameter object
*/
this.exec = function(apiID,module,parameters)
    {
    this.parameters = parameters;
    this.setModule(module);
    if(apiID) this.load(apiID);
    this.render();
    }


/**
* Dumps raw JSON into slot
*/
this.showRAW= function()
    {
    this.setModule('dumper');
    this.render();
    }


/**
* Sets slot (div) content
* @parameter html {string} 
*/
this.setContent = function(html)
    {
    // show slot
    this.show();
    // collect content
    var out = "";
    if(this.title) out+="<h1>"+this.title+"</h1>\n";
    out+=html;
    if(this.menu) out+="\n"+this.dumpMenu();
    if(this.closeable) out+='<img src="gfx/icons/close.png" alt="close" border="0" width="24" height="24" class="close" onclick="'+this.name+'.hide();">\n';
    
    // set content
    this.target.innerHTML = out;
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
    this.target.innerHTML = '<div class="message">'+msg+'</div>';
    }


/**
* Adjusts custom DIV width
*/     
this.setWidth = function(w) { this.target.style.width = String(w)+'px'; }    
/**
* Adjusts custom DIV height
*/     
this.setHeight = function(h) { this.target.style.height = String(h)+'px'; }    

    
/**
* Creates menubar 
*/
this.dumpMenu = function() {  return iconMenu(this.menu); }
    
    
}
