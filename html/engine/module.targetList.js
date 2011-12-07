/**
* ******* RTS UI LIST VIEW CLASS *******
* General table dumping class
* Dumps a table from given JSON data
* CSS classnames for data styling defined by .colClasses array for each column
* If colClass = "BLOCK" the collumn is ommited.
* Functions for cell data processing defined by .colFX arrat for each column
* @constructor
*/

// inherit table class (see inc.commons.js)
copyPrototype(targetList,table);

// descendant
function targetList(slotInstance)
{

this.table(slotInstance);

// menu
this.slot.menu =
    [
    ["find.png",'Map view',"side.setModule('tarFilter');side.render();"],
    ];

this.slot.title = "Target list";
    
// column classes
this.colClasses = new Array();
this.colClasses = { 0:"", 1:"small right", 2:"bold", 3:"timestamp", 4:"timestamp"};

// row actions
this.rowActions = function(row)
        {
        var click =  "side.exec(null,'targetPreview',{index:'"+String(row)+"',data:"+this.slot.name+".data})";
        var over = "hint('click for preview');hi(this);";
        var out =  "back(this);";
        return 'onclick="'+click+'" onmouseover="'+over+'" onmouseout="'+out+'"';
        }

        
/**
* Function to receive event
* @parameter id {string} event id
* @parameter data {object} data to process
*/
this.event = function(id,data)
    {
    switch(id)
        {
        // date picked from calendar
        case 'filter':
            
            this.filters = new Array(); // reset filters
            
            var val = null;
            val = getValue('filter.text');
            if(val) { this.filters.push({column:_APIDATA.TARGETS.NAME,type:'fulltext',value:val});}

            val = getValue('filter.idFrom');
            if(val) { this.filters.push({column:_APIDATA.TARGETS.ID,type:'gt',value:val});}

            val = getValue('filter.idTo');
            if(val) { this.filters.push({column:_APIDATA.TARGETS.ID,type:'lt',value:val});}
            
            this.render();

            break;

        }

    }
        

}
  

