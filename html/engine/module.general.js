/* ================== GENERAL MODULES =============== */
/**
* ******* RTS UI BLANK MODULE CLASS *******
* Dumps content given by parameters
* @constructor
*/
function transfer(slotInstance)
{
this.slot = slotInstance;

// clear all
this.slot.menu = null;
this.slot.title = "";
this.slot.closeable = true;

this.render = function()
    {
    // no data
    if(!this.slot.parameters) {this.slot.setContent('No data');return;}

    // set title
    if(this.slot.parameters.title) {this.slot.title = this.slot.parameters.title;}

    // set DIV dimensions
    if(this.slot.parameters.width) {this.slot.setWidth(this.slot.parameters.width);}
    if(this.slot.parameters.height) {this.slot.setHeight(this.slot.parameters.height);}

    // content
    if(this.slot.parameters.content) {this.slot.setContent(this.slot.parameters.content);}
    }

}
  

/**
* ******* RTS UI DUMPER CLASS *******
* Simply dumps api response content to div 
* given DIV block using given parsing module
* @constructor
*/
function dumper(slotInstance)
{
this.slot = slotInstance;

this.render = function()
    {
    var raw = this.slot.loader.rq.responseText;
    raw = raw.replace(/</g,'&lt;')
    raw = raw.replace(/>/g,'&gt;')
    this.slot.setContent('<h1>Raw data</h1><pre>'+raw+'</pre>');
    }

}
  

  

