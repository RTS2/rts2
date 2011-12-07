/**
* ******* RTS UI DATE SELECTOR CLASS*******
* @constructor
*/
function daySelector(slotInstance)
{

// target slot
this.slot = slotInstance;

// slot to control
var slave =  null;


// calendar instance
this.calendar = null;

this.jsdpConfig = 
        {                                           
            useMode:1,
            isStripped:true,
            cellColorScheme:"orange",
            imgPath:"gfx/cal/",
            selectedDate:{      
                day:1,          
                month:1,
                year:2011
            },
            target:"calDiv"
            /*yearsRange:[1978,2020],
            limitToToday:false,
            cellColorScheme:"beige",
            dateFormat:"%Y/%m/%d",
            weekStartDay:1*/
        }


this.render = function()
    {
        var d = new Date();
        
        
        // DIV setup
        this.slot.setHeight(220);
        this.slot.closeable = true;
      
        this.slot.title = "Pick date";
        this.slot.setContent('<div id="calDiv"></div>');
      
        // parameters
        this.slave = this.slot.parameters.slave;
        if(this.slave)
            {
            if(this.slave.parameters.date) {d = this.slave.parameters.date;}
            }
            
        //  set date
        this.jsdpConfig.selectedDate.day = d.getDate(); 
        this.jsdpConfig.selectedDate.month = d.getMonth()+1; 
        this.jsdpConfig.selectedDate.year = d.getFullYear(); 
      
        // render calendar
        this.calendar = new JsDatePick(this.jsdpConfig);    
        this.calendar.setSelectedDay(this.jsdpConfig.selectedDate);
        
        // set object to callback
        this.calendar.slave = this.slave;
        
        // set handler                      
        this.calendar.setOnSelectedDelegate(this.datePicked);
 

    }

this.datePicked = function(sender)
    {
    var sd = sender.getSelectedDay();
    var d = new Date(0);
    d.setFullYear(Number(sd.year));
    d.setMonth(Number(sd.month)-1);
    d.setDate(Number(sd.day));
    sender.slave.event('newdate',{date:d});
    }
}
  

  

