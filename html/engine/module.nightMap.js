/**
* ******* PLAN VISUALISATION CLASS *******
* @parameters {DOM element} content element
* @constructor
*/
function nightMap(slotInstance)
{
    
// target slot
this.slot = slotInstance;

// BASE
this.base = dateNoon(this.slot.parameters.date);    // base (moment) ... typically basedate 12:00 local

// div width
//this.slot.setWidth(720);

/**
* map of the day/night
*
* map[0] ~ base (typically basedate 12:00) 
* map[1440] ~ base+24h
* 1 min/cell 
*
* values:
*    -1~astronomical tw
*    -2~nautical tw
*    -3~civil tw
*    -4~sunset/rise >0 target id
*/
this.map = new Array();

this.slot.menu =  // replace "main." with this.slot.name if module has to be used in other slot than main
    [
    ["astro.png","Twilight info","side.exec(null,'transfer',{title:'Twilight info',height:180,content:main.module.sunInfo()});"],
    ["list.png",'List view',"main.setModule('finishedList');main.render();"],
    ["calendar.png","Pick date","side.exec(null,'daySelector',{slave:main});"],
    ];


// target preview DIV
this.preview = null;

// SUN - rise, set, twilights - filled by .sunCalc()
this.sunset = 0;
this.sunrise = 0;
this.astroB = 0; 
this.astroE = 0;
this.nautiB = 0;
this.nautiE = 0;
this.civilB = 0;
this.civilE = 0;


/**
* Returns index of the map cell for given Date() [UTC] object. Return -1 if date is out of map range.
* @parameter utc {Date object}
*/
this.utcToIX= function(utc)
    {
    // get ms
    var basems = this.base.getTime();
    var utcms = utc.getTime();
    // calc
    var cix = (utcms+_TIMEZONE*3600000-basems)/60000;
    // test ranges
    if(cix<0) {return -1;}
    if(cix>1440) {return -1;}
    return Math.round(cix);    
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
        case 'newdate':
            this.slot.parameters.date = data.date;
            this.slot.reload();
            this.slot.render();
            break;
        // target preview (map hover)
        case 'preview':
            if(this.preview) this.preview.innerHTML = data.text;
            break;
        }

    }


    

/**
* Fills given range of the map by value(target ID)
* @parameter from {Date object} interval begin - UTC!
* @parameter to {Date object} interval end - UTC!
* @parameter val {int} value to store
* @return success {boolean} false if date out of range
*/
this.mapFill= function(from,to,val)
    {
    // convert to map indexes
    var fix = this.utcToIX(from);
    var tix = this.utcToIX(to);
    // test ranges 
    if((fix<0)||(tix<fix)) {return false;}
    // fill the map
    for(var i=fix;i<tix;i++) { this.map[i]=val}
    return true;
    }



/**
* Dumps the night map
*/
this.render = function()
    {
    var out = "";

    // global parameters
    var gp = this.slot.parameters;
    if(gp)
        {
        if(gp.date) {
                        this.base = dateNoon(gp.date); // shift to noon
                        }
    
        }
    
    // clean map
    this.map = new Array(); 
    for(var i=0;i<1440;i++) {this.map[i]=0;}

    // calc the sun
    this.sunCalc(this.base.getDate(),this.base.getMonth()+1,this.base.getFullYear());

    // twilight fill
    this.mapFill(dateAddHrs(this.base,this.sunset-12),dateAddHrs(this.base,this.sunrise+12),-4);
    this.mapFill(dateAddHrs(this.base,this.civilB-12),dateAddHrs(this.base,this.civilE+12),-3);
    this.mapFill(dateAddHrs(this.base,this.nautiB-12),dateAddHrs(this.base,this.nautiE+12),-2);
    this.mapFill(dateAddHrs(this.base,this.astroB-12),dateAddHrs(this.base,this.astroE+12),-1);
    
    
    // targets fill
    if(!this.slot.data) {this.slot.message("<b>No data to show.</b>"); return false;}
    var DATA = this.slot.data.d;

    var rows = DATA.length;   
     
    var malformed = 0;   // error counters
    var outOfRange = 0;
    
    for(var row=0;row<rows;row++)
            {
            var d = DATA[row]; // get from data
            var from = strToDateTime(d[_APIDATA.FINISHED.FROM]);
            var to = strToDateTime(d[_APIDATA.FINISHED.TO]);               
            if((from==null)||(to==null)) { malformed++; continue; }
            if(!this.mapFill(from,to,row+1000)) { outOfRange++; continue;}
            }
    
    // title
    this.slot.title = "Night map: "+dateStr(this.base);
    
    out+='<div style="margin: 10px 0px 10px 0px"><i>Target:</i> <span id="mapTargetPreview" style="font-weight:bold">-</span></div>\n';
    
    // map dump
    out+='<table class="nightMap" border="0" cellpadding="0" cellspacing="0">\n';

    // header
    out+='<tr>\n';
    out+='<th class="local" colspan="2">Site time</th>\n';
    for(col=0;col<6;col++)
            {
            out+='<th class="min" colspan="10">'+col*10+' min</th>\n';
            }    
    out+='<th class="utc">UTC</th>\n';
    out+='<th class="utc">Local</th>\n';
    out+='</tr>\n';
    
    // data
    var datelabel = "";
   
    var occupCounter = 0;  // zig-zag coloring 
    var lastID = 0; 
    var span = 0;       // spanning 
    var i = 0; 
    
    for(row=0;row<24;row++)
        {
        var ccAux=null; // aux classing variable
        
        out+='<tr>\n'; // make map row

        rowdt = dateAddHrs(this.base,row); // compute row datetime
        
        // print datelabel if changed
        var newdatelabel = dateStr(rowdt);
        if(newdatelabel!=datelabel)
            {
            datelabel = newdatelabel;
            out+='<th class="datelabel">'+datelabel+'</th>\n';
            ccAux = "delim";
            }
            else
            {out+='<th class="dateblank">&nbsp;</th>\n';}
            
        // local time
        out+='<th class="local '+ccAux+'">'+dateHrs(rowdt)+'h</th>\n';

        // cells
        for(col=0;col<60;col++)
            {
            var id = this.map[row*60+col];
            // cell styling
            var cc=null;
            switch (id)
                {
                case -4:cc="riset";break;
                case -3:cc="civil";break;
                case -2:cc="nauti";break;
                case -1:cc="astro";break;
                case  0:cc=null;break;
                default:
                    cc="occup";
                    if(id!=lastID) { occupCounter++; lastID = id; blockSize=0;}
                            else   { blockSize++;}
                    cc = (occupCounter%2==0)?'occupEven':'occupOdd';
                    
                    break;
                }
            var cellClass=' class="'+(cc?cc:'')+' '+(ccAux?ccAux:'')+'"';
         
         
            if(id>0)
                { // occupied cell
                var index = id-1000;
                var d = DATA[index];
                
                //out+='<td'+cellClass+' colspan="'+span+'">&nbsp;</td>\n';   // output
                if(span>1) {span--;} //skip if spanning
                    else
                    { // new cell
                   
                    // count span
                    for(span=1,i=col+1;i<60;i++) { if(this.map[row*60+i]==id) {span++;} }
                    // cell caption  (inner text)
                    var celltext = stringLimit(d[2],span);
                    //cell actions
                    var onclick =  "side.exec(null,'targetPreview',{index:'"+String(index)+"',data:"+this.slot.name+".data})";
                    var onmouseover =  this.slot.name+".event('preview',{text:'"+d[_APIDATA.FINISHED.NAME]+"'});hint('click for preview');hi(this);";
                    var onmouseout =  this.slot.name+".event('preview',{text:'-'});back(this);";
                    // the cell                    
                    out+='<td'+cellClass+' colspan="'+span+'" onclick="'+onclick+'" onmouseover="'+onmouseover+'" onmouseout="'+onmouseout+'" >'+celltext+'</td>\n';   // output
                    }
                }
                else
                { // blank cells
                out+='<td'+cellClass+'>&nbsp;</td>\n';   // output 
                }
            }
        
        // secondary time scales
        var utc = dateAddHrs(rowdt,-_TIMEZONE)    
        var local = dateAddHrs(utc,-utc.getTimezoneOffset()/60);
        
        out+='<th class="times utc '+ccAux+'">'+dateHrs(utc)+'h</th>\n';
        out+='<th class="times utc '+ccAux+'">'+dateHrs(local)+'h</th>\n';
        out+='</tr>\n';
        }
    out+='</table>\n';
    

    // error report
    if(malformed+outOfRange>0)
        {
        var warnMessage = malformed+' malformed records, \n'+ outOfRange+' records out of time range.\n';
        out+='<div class="errorLine">\n'+warnMessage+'</div>\n';
        status('WARNING: '+warnMessage);
        }
    
    
    // output
    this.slot.setContent(out);
    
    // set preview div
    this.preview = document.getElementById('mapTargetPreview'); 
    }



/**
* Dumps sun+darkness information
*/
this.sunInfo = function()
    {
    var out = "";
    out+= '<table align="center">';
    out+='<tr><th>Sunset:</th><td>'+hm(this.sunset)+'</td></tr>\n';
    out+='<tr><th>Civil:</th><td>'+hm(this.civilB)+'</td></tr>\n';
    out+='<tr><th>Nautical:</th><td>'+hm(this.nautiB)+'</td></tr>\n';
    out+='<tr><th>Astro:</th><td>'+hm(this.astroB)+'</td></tr>\n'; 
    out+='<tr><th>Astro:</th><td>'+hm(this.astroE)+'</td></tr>\n';
    out+='<tr><th>Nautical:</td><td>'+hm(this.nautiE)+'</td></tr>\n';
    out+='<tr><th>Civil:</th><td>'+hm(this.civilE)+'</td></tr>\n';
    out+='<tr><th>Sunrise:</th><td>'+hm(this.sunrise)+'</td></tr>\n';
    out+= "</table>";
    out+= '<hr><div align="center"><b>Date: '+dateStr(this.base)+'</b></div>';
    return out;
    }
    
    
/**
* Sunrise/set and twilight calculator
*
* Source:
*    Almanac for Computers, 1990
*    published by Nautical Almanac Office
*    United States Naval Observatory
*    Washington, DC 20392
*    http://williams.best.vwh.net/sunrise_sunset_algorithm.htm
*/
this.sunCalc = function(day,month,year)
    {
    // CONSTANTS
    var riseset      = 90.5;
    var civil        = 96;
    var nautical     = 102;
    var astronomical = 108;
    
    var rise = true;
    var set = false;
    
    var RADDEG = 180/Math.PI;
    var DEGRAD = Math.PI/180;

    
    // SETUP ... from config
    var timezone = _TIMEZONE;
    var latitude = _LATITUDE;
    var longitude = _LONGITUDE; 


    function calc(day,month,year,zenith,rs)
    {
    // first calculate the day of the year
    var N1 = Math.floor(275 * month / 9);
    var N2 = Math.floor((month + 9) / 12);
    var N3 = (1 + Math.floor((year - 4 * Math.floor(year / 4) + 2) / 3));
    var N = N1 - (N2 * N3) + day - 30;

    //convert the longitude to hour value and calculate an approximate time

    var lngHour = longitude / 15;
    
    var t;
    if(rs) {t = N + ((6 - lngHour) / 24);} // rise
           else
           {t = N + ((18 - lngHour) / 24);} // set

    // calculate the Sun's mean anomaly
             
    var M = (0.9856 * t) - 3.289;
    
    // calculate the Sun's true longitude
    
    L = M + (1.916 * Math.sin(DEGRAD*M)) + (0.020 * Math.sin(2 * DEGRAD*M)) + 282.634;
    L = norm360(L);

    // calculate the Sun's right ascension
    
    var RA = RADDEG*Math.atan(0.91764 * Math.tan(DEGRAD*L));
    RA = norm360(RA);

    // right ascension value needs to be in the same quadrant as L

    var Lquadrant  = (Math.floor( L/90)) * 90;
    var RAquadrant = (Math.floor(RA/90)) * 90;
    RA = RA + (Lquadrant - RAquadrant);

    // right ascension value needs to be converted into hours

    RA = RA / 15;

    // calculate the Sun's declination

    var sinDec = 0.39782 * Math.sin(DEGRAD*L);
    var cosDec = Math.cos(Math.asin(DEGRAD*sinDec));
       
    // calculate the Sun's local hour angle
    var cosH = (Math.cos(DEGRAD*zenith) - (sinDec * Math.sin(DEGRAD*latitude))) / (cosDec * Math.cos(DEGRAD*latitude));
    
    if (cosH >  1) {out = -1;} //the sun never rises on this location (on the specified date)
    if (cosH < -1) {out = -1;} //the sun never sets on this location (on the specified date)

    //finish calculating H and convert into hours
    var H;
    if(rs)  {H = 360 - RADDEG*Math.acos(cosH);} // rise
            else
            {H = RADDEG*Math.acos(cosH);} // set
    H = H / 15;

    // calculate local mean time of rising/setting
    var T = H + RA - (0.06571 * t) - 6.622;

    // adjust back to UTC
    
    var UT = T - lngHour;
    UT = norm24(UT);

    return UT;
    
    //convert UT value to local time zone of latitude/longitude
    //var localT = UT + timezone;
    //return localT;    
    } // calc
    
    

    this.sunset = calc(day,month,year,riseset,set);
    this.sunrise = calc(day,month,year,riseset,rise);
        
    this.astroB = calc(day,month,year,astronomical,set);
    this.astroE = calc(day,month,year,astronomical,rise);
        
    this.nautiB = calc(day,month,year,nautical,set);
    this.nautiE = calc(day,month,year,nautical,rise);
        
    this.civilB = calc(day,month,year,civil,set);
    this.civilE = calc(day,month,year,civil,rise);
    
    
    } // suncalc
    
    
    
    
    
    
    
    
} // nightMap class
