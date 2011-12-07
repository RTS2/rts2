/**
* ******* RTS UI LOGIN SCREEN CLASS *******
* Login screen
* @constructor
*/


function login(slotInstance)
{
this.slot = slotInstance;

this.slot.setWidth(680);

// menu
this.slot.menu =
    [
    ["info.png","Server info","main.event('serverinfo',null);"],
    ["help.png","General help","main.event('help',null);"],
    ];


this.render = function()
    {
    this.slot.title = "Server authorization";
    var out = "";
    
    out+='<div align="right">Server: <b>'+_server+'</b><hr>';
    out+='<form name="auth" method="get" onsubmit="message">';
    out+='User name: <input type="text" name="user" value="'+_user+'" size="20">&nbsp;&nbsp;&nbsp;';
    out+='Password: <input type="password" name="pswd" value="'+_passwd+'" size="20">&nbsp;&nbsp;&nbsp;';
    out+='<input type="submit" value="Set">';
    out+='</form></div>';
    
    this.slot.setContent(out);
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
        case 'serverinfo':
            var text = 
                    '<p>Lorem ipsum dolor sit amet consectetuer accumsan sem pellentesque ut pretium. Quisque gravida id in sit feugiat lacinia purus lacus tempor id. Tempor Aenean venenatis fermentum In interdum laoreet enim et at pretium. Nullam vitae condimentum est ligula et Curabitur quis laoreet scelerisque eget. Tempor sociis Suspendisse urna sed nunc Phasellus sed nibh enim Pellentesque. Fusce at nunc.</p>'+
                    '<p>In ante velit metus sed dui quis laoreet non pellentesque Donec. Porta neque magna auctor eros elit a penatibus Nunc ac semper. Vitae pede tellus tincidunt libero est dictum ut sed vitae elit. Interdum vestibulum libero pede fermentum hendrerit Curabitur lacinia laoreet eleifend tortor. Phasellus libero est tempus quis Curabitur Phasellus consequat Suspendisse feugiat id. Sed Phasellus hendrerit nec eget Curabitur et dolor Pellentesque neque.</p>'+
                    '<p>Quis sociis Vivamus venenatis vitae nec Nulla consectetuer cursus et nulla. Semper congue sodales sem interdum sed Nullam diam leo dis fringilla. Adipiscing condimentum orci nibh elit at turpis interdum vitae vel Phasellus. Et sit pharetra arcu risus egestas gravida Vestibulum felis sem tristique. Tortor enim Donec id eu laoreet malesuada eros ante quam Phasellus. Dui sem sed Morbi.</p>';
            aux.setWidth(680);
            aux.exec(null,'transfer',{title:'Server info',content:text});
            break;

        case 'help':
            var text = 
                    '<div style="margin:5px 0px 10px 0px"><i>Main menu quick help.</i></div>'+

                    '<table cellpadding="5" cellspacing="5" border="0">'+
                    '<tr><td><div class="icon"><img src="gfx/icons/editor.png" border="0" width="32" height="32"></div></td>'+
                    '<td>Target browser and editor.</td></tr>'+

                    '<tr><td><div class="icon"><img src="gfx/icons/finished.png" border="0" width="32" height="32"></div></td>'+
                    '<td>Finished observations review.</td></tr>'+

                    '<tr><td><div class="icon"><img src="gfx/icons/plan.png" border="0" width="32" height="32"></div></td>'+
                    '<td>Observation planner.</td></tr>'+

                    '<tr><td><div class="icon"><img src="gfx/icons/setup.png" border="0" width="32" height="32"></div></td>'+
                    '<td>Settings and options.</td></tr>'+

                    '<tr><td><div class="icon"><img src="gfx/icons/login.png" border="0" width="32" height="32"></div></td>'+
                    '<td>Login screen.</td></tr>'+
                    '</table>';


            side.exec(null,'transfer',{title:'Help',content:text});
            break;



        }

    }

  


}
  

