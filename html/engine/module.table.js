/**
* ******* RTS UI TABLE VIEW CLASS *******
* General table dumping class
* Dumps a table from given JSON data
* CSS classnames for data styling defined by .colClasses array for each column
* If colClass = "BLOCK" the collumn is ommited.
* Functions for cell data processing defined by .colFX array for each column
* @constructor
*/
function table(slotInstance) {

// target slot
this.slot = slotInstance;
this.slot.title = "Table view";
}

// functions to process(output) cell content 
table.prototype.colFX = {};

// column classes
table.prototype.colClasses = new Array();

/**
* array of filters
* filter = {column:x, type:<type>, value:v}
*   column: processed col
*   type: 'fulltext', 'gt' (>), 'lt' (<)
*/
table.prototype.filters = new Array();

/**
* defines row user interaction 
* @parameter row {int} row index
* @returns {string} HTML event string
*/
table.prototype.rowActions = function(row)
        {
        /* DEMO
        var click =  "side.exec(null,'targetPreview',{index:'"+String(row)+"',data:"+this.slot.name+".data})";
        var over = "hint('click for preview');hi(this);";
        var out =  "back(this);";
        return 'onclick="'+click+'" onmouseover="'+over+'" onmouseout="'+out+'";
        */
        return "";
        }



/**
* Parse + dump
*/
table.prototype.render = function()
    {

    // init
    var out = '';
    var data = this.slot.data;
    if(!data) { return false;}
    
    // dimensions
    var columns = data.h.length;
    var rows = data.d.length;
    
    
    // table
    out+='<table class="dataTable" border="0" cellpadding="0" cellspacing="0">\n';
    // header
    out+='<tr>\n';
    for(col=0;col<columns;col++)
            {
            if(this.colClasses[col]=="BLOCK") {continue;} // if column is blocked go ahead
            out+='<th>'+data.h[col].n+'</th>\n';
            }    
    out+='</tr>\n';
    // data
    for(row=0;row<rows;row++)
        {
        var rowData = cellcontent = data.d[row]; // get from data

        //filtering
        var passed = true;
        for(var fIX=0;fIX<this.filters.length;fIX++) // process filters
            {
            var filter = this.filters[fIX];
            switch(filter.type)
                {
                case 'fulltext':
                    if(String(rowData[filter.column]).search(RegExp(filter.value,'i'))<0) {passed=false;}
                    break; 
                case 'gt':
                    if(rowData[filter.column]<filter.value) {passed=false;}
                    break; 
                case 'lt':
                    if(rowData[filter.column]>filter.value) {passed=false;}
                    break; 
                }
            }
        if(!passed) continue; // next row if not passed the filter
            
            
        //row dump
        out+='<tr  '+this.rowActions(row)+'>\n';
        for(col=0;col<columns;col++)
            {
            // cell classing
            classes = this.colClasses[col]; // user defined classes
            if(classes=="BLOCK") {continue;} // if column is blocked go ahead
            classes += (row%2==0)?" odd":""; // odd/even lines
            if(classes) { cellclass=' class="'+classes+'"';}

            // process content
            cellcontent = data.d[row][col]; // get from data
            if(this.colFX[col]) { cellcontent = this.colFX[col](cellcontent); } // process by custom function if defined
            
            // dump the cell
            out+='<td'+cellclass+'>'+cellcontent+'</td>\n';
            }
        out+='</tr>\n';
        }
    out+='</table>\n';
    
    // output
    this.slot.setContent(out);
    
    // status
    status('Ready');
    }

    
 

