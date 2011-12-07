/**
* ******* RTS UI WORKSPACE CONFIGURATOR *******
* UI views:
*     - target editor
*     - observation browser
*     - planner
*     - setup
*     - autorization/login
* @constructor
*/
function workbench() 
{
this.activeView = null;

/**
* Workbench-mode definition table
*/
this.wbConfig = [
          { id:'editor', icon:'editor.png', hint:'Target editor'},
          { id:'finished', icon:'finished.png',hint:'Finished observations'},
          { id:'planner', icon:'plan.png', hint:'Planner'},
          { id:'setup', icon:'setup.png', hint:'Setup'},
          { id:'login', icon:'login.png', hint:'Login'}
        ];

this.count = this.wbConfig.length;
        
/**
* Prepares general menu bar
* (mode icons)
*/
this.menu = function()
    {
    var out='<div class="menu">';
    for(var i=0;i<this.count;i++)
        {
        out+='<div class="icon" onmouseover="hi(this);hint(\''+this.wbConfig[i].hint+'\');" onmouseout="back(this);" onclick="wb.switchView(\''+this.wbConfig[i].id+'\')" id="wbModeButton'+this.wbConfig[i].id+'">';
        out+='<img src="gfx/icons/'+this.wbConfig[i].icon+'" border="0" width="32" height="32"></div>';
        }
    
    out+="</div>";
    return out;
    }
    
    
/**
* table of icon-buttons
* filled on the 1st run 
*/
this.iconButtons = null;

/**
* activate one of the mode icons
* ... deactivate the rest
* @paramater viewID {string} view id
*/
this.buttonActivate = function(aix)
    {
    var i = 0;
    
    // fill the array on the first run                                                             
    if(!this.iconButtons) 
        {
        this.iconButtons = new Array();
        for(i=0;i<this.count;i++) {
                        this.iconButtons[i] = document.getElementById('wbModeButton'+this.wbConfig[i].id);
                        }
        }
    
    // activate selected
    for(i=0;i<this.count;i++)
            { 
            this.iconButtons[i].className = (i==aix)?"icon active":"icon"; 
            if(i==aix) this.iconButtons[i].cBackup = "icon active";
            }
    }
    

/**
* Switch view
* - desktop setup
* - init slots
* - load default content
*/
this.switchView = function(viewID)
    {
    var vIX = -1;
    // search ... set vIX    
    for(i=0;i<this.count;i++) {if(this.wbConfig[i].id == viewID) {vIX=i;break;} }
    if(vIX<0) {status('Workbench: Invalid viewID');return;}
    
    // activate button
    this.buttonActivate(vIX);
    
        
    // reset slots
    main = new slot("main");
    main.hide();
    side = new slot("side");
    side.hide();
    aux = new slot("aux");
    aux.hide();    
    
    // main switching board
    switch(viewID)
        {
        case 'finished':  
              main.show();
              main.exec('finished','nightMap',{date: new Date("2010/9/27")});
              break;
        
        case 'editor': 
              main.show();
              main.exec('targets','targetList',{date: new Date("2010/9/27")});
              break;
        
        case 'login': 
              main.show();
              main.exec(null,'login',null);
              break;
        
        
        
        
        default: status('ERROR: Invalid view ID - '+viewID);break;
        }
    }


/**
* Server info summary
*/
this.serverInfo = function()
    {
    var out = "";
    out+='Server: <b>'+_server+'</b><br>';
    out+='User: <b>'+_user+'</b><br>';
    return out;
    }
    
}
