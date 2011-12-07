/**
* RTS UI global classes, vars and functions
*/

/**
* Status DIV
*/
var statusDiv = null;

/**
* Hint DIV
*/
var hintDiv = null;

/**
* Update status message
*/ 
function status(text)
    {
    if(!statusDiv) { statusDiv = document.getElementById('status'); }
    statusDiv.innerHTML = text;
    }

/**
* Displays tool hint
*/ 
function hint(text)
    {
    if(!hintDiv) { hintDiv = document.getElementById('hint'); }
    hintDiv.innerHTML = text;
    }

    
/**
* Get form-input value (SAFE)
* @parameter id {string} DOM id
* @returns val {string}
*/
function getValue(id)
    {
    var input = document.getElementById(id);
    if(input) { return input.value;} else return null;
    }    

    
/* ******************* ICON MENU ******************* */
/**
* dumps icon menu bar
* @parameter menu {array} menu definition
*
* Menu - array of [iconfile,caption,href/js-code]
* defining one buttonbox 
*/
function iconMenu(menu)
    {
    var out='<div class="menu">';
    for(var i=0;i<menu.length;i++)
        {
        var iconfile = menu[i][0];
        var caption = menu[i][1];
        var action = menu[i][2];
        out+='<div class="icon" onmouseover="hi(this);hint(\''+caption+'\');" onmouseout="back(this);" onclick="'+action+'">';
        out+='<img src="gfx/icons/'+iconfile+'" border="0" width="32" height="32"></div>';
        }
    
    out+="</div>";
    return out;
    }

    
    
/* ***************** GLOBAL HANDLERS ***************** */
// hilights UI element (on mouse over)
function hi(sender)
    {
    sender.cBackup = sender.className;
    sender.className = sender.cBackup+' hi';
    }

// returning the element class(es) back (on mouse out)
function back(sender)
    {
    sender.className = sender.cBackup;
    }

/* *********** CLASS INHERITANCE SIMULATOR ********** */
/**
* Class inheritance :simulator:
* copies class prototype to target class
* source: http://blogs.sitepoint.com/2006/01/17/javascript-inheritance/
*/
function copyPrototype(descendant, parent) {
    var sConstructor = parent.toString();
    var aMatch = sConstructor.match( /\s*function (.*)\(/ );
    if ( aMatch != null ) { descendant.prototype[aMatch[1]] = parent; }
    for (var m in parent.prototype) {
        descendant.prototype[m] = parent.prototype[m];
    }
};