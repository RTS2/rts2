/* ============== DATETIME ============= */

/**
* Prints date part of Date() object
* @parameter d {Date} date object
* @returns {string} YYYY/MM/DD
*/
function dateStr(d)
    {   
    return(d.getFullYear()+'/'+(d.getMonth()+1).toString()+'/'+d.getDate());
    }
 
/**
* Prints hours (local) of Date() object
* @parameter d {Date} date object
* @returns {string} hours
*/
function dateHrs(d) {return(d.getHours());}

/**
* Add hours to Date() object
* @parameter d {Date} date object
* @parameter h {int} hours to add
* @returns {Date object}
*/
function dateAddHrs(d,h)
    {
    return(new Date(d.getTime()+ 3600000*(h)));
    }

/**
* Returns noon of the given Date
* @parameter d {Date} date object
* @parameter h {int} hours to add
* @returns {Date object}
*/
function dateNoon(d)
    {
    var out = new Date(d);
    out.setHours(12);
    out.setMinutes(0);
    out.setSeconds(0);
    out.setMilliseconds(0);
    return out;
    }

    
/**
* Makes Date object from API-string (YYYY/MM/DD HH:MM:SS.SSS UT)
* @parameter s {string}
* @returns {Date object}
*/
function strToDateTime(dat)
    {
    var s = new String(dat);
    var d = new Date();
    var re = /([0-9]{4})\/([0-9]{2})\/([0-9]{2}) ([0-9]{2}):([0-9]{2}):([0-9]{2}).([0-9]{3})/;
    var ex = s.toString().match(re);
    if(!ex){ return null;}
    d.setFullYear(Number(ex[1]));
    d.setMonth(Number(ex[2])-1);
    d.setDate(Number(ex[3]));
    d.setHours(Number(ex[4]));
    d.setMinutes(Number(ex[5]));
    d.setSeconds(Number(ex[6]));
    d.setMilliseconds(Number(ex[7]));
    return d;
    }

/**
* Makes Date object from API unix timestamp
* @parameter s {int}
* @returns {Date object}
*/
function unixDate(dat)
    {
    var d = new Date();
    d.setTime(dat*1000);
    return d;
    }

/**
* Makes human readable string from API unix timestamp
* @parameter s {int}
* @returns {string}
*/
function unixDateStr(dat)
    {
    if(dat==null) return "-";
    var d = unixDate(dat);    
    return '<span class="timestamp">'+d.toString()+"</span>";
    }
    
/**
* Makes Date object date string (YYYY/MM/DD) 
* @parameter s {string}
* @returns {Date object}
*/
function strToDate(s)
    {
    var d = new Date(0);
    var re = /([0-9]+)\/([0-9]+)\/([0-9]+)/;
    var ex = s.match(re);
    if(!ex){ return null;}
    d.setFullYear(Number(ex[1]));
    d.setMonth(Number(ex[2])-1);
    d.setDate(Number(ex[3]));
    return d;
    }

    
/**
* Hours (float) to h:mm string
* @parameter x {float} hours
* @returns {string}
*/
function hm(x) {
                var m = Math.floor(x).toString();
                var s = Math.floor(x*60%60);
                if(s<10) {s = '0'+s.toString();} else {s=s.toString();}
                return m+':'+s;
                 }
    

/**
* Limits string to given number of character
* @parameter s {string} source
* @parameter n {int} length
* @returns {string}
*/
function stringLimit(s,n)
    {
    var l = s.length; 
    if(l>n)
        {  // longer than limit
        if(l>2)  {return(s.substr(0,n-2)+' ~');}  // append ~
            else {return(s.substr(0,n)); }      // extra short
        }
        else
        {return s;} // in size
    }


    

/* ============== NORMALISATION ============= */
    
/**
* Number normalisation - base 360 (degrees)
* @parameter L {float} 
* @returns {float} 
*/    
function norm360(L)
        {
        //if(L>=360) {L=L-360;} 
        if(L<0) {L=L+3600;}
        return L%360;
        }

        
/**
* Number normalisation - base 24 (hours)
* @parameter L {float} 
* @returns {float} 
*/    
function norm24(L)
        {
        //if(L>=24) {L=L-24;} 
        if(L<0) {L=L+240;}
        return L%24;
        }
