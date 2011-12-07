/**
* ******* RTS UI FINISHED TARGET SEARCH MODULE CLASS *******
* Login screen
* @constructor
*/


function tarFilter(slotInstance)
{
this.slot = slotInstance;

this.render = function()
    {
    this.slot.title = "Filter";
    
    var out = '\
    <div align="center" style="margin-top:10px;">\
    Name: <input type="text" id="filter.text" value="" size="20">\
    <div align="center" style="margin-top:10px;">\
    <input type="text" id="filter.idFrom" value="" size="4">&lt;\
    ID\
    &lt<input type="text" id="filter.idTo" value="" size="4">\
    </div>\
    <hr>\
    <input type="button" value="Filter" size="4" onClick="main.event(\'filter\',null)">\
    </div>\
    ';
    
    this.slot.setContent(out);
    }



}
  

