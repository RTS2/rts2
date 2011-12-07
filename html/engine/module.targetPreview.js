/**
* ******* RTS UI TARGET PREVIEW CLASS *******
* Displays <data>[<index>] ~ row of api response 
* <data> and <index> loaded from .slot.parameters 
*(typically loaded by slot.exec)
* @constructor
*/
function targetPreview(slotInstance)
{
this.slot = slotInstance;

this.colFX = {3:unixDateStr,4:unixDateStr,5:unixDateStr};  


// div setup
this.slot.closeable = true;

this.render = function()
    {
    this.slot.title = "Preview";

    if((!this.slot.parameters)||(!this.slot.parameters.data))
        { this.slot.setContent('No data to preview.'); return;}

    
    var data = this.slot.parameters.data;
    var index = this.slot.parameters.index;
    var out = "";
 
    out+= '<table align="center">\n';
    for(var i=0;i<data.h.length;i++)
        {
         var cellcontent;        
         cellcontent = data.d[index][i]; // get from data
         if(this.colFX[i]) { cellcontent = this.colFX[i](cellcontent); } // process by custom function if defined
        
        out+='<tr><th><i>'+data.h[i].n+':</i></th><td><b>'+cellcontent+'</b></td></tr>\n'
        }
    out+= '</table>\n';
    out+= '<hr>';  
 
     var menu = [
          ['edit.png','Edit',''],
          ['images.png','Images',''],
        ];
    
    out+='<div align="center">'+iconMenu(menu)+'</div>';

    
    
        
    this.slot.setContent(out);


    }

}
