/*
	Copyright 2009 Itamar Arjuan
	jsDatePick is distributed under the terms of the GNU General Public License.
=====================================================================================
Hardly modified by M.i.X - do not upgrade this library!!!!!!
- image path changed (js,CSS)
- date processing modifications (setdate)
- pick event (OnSelectedDelegate) sends self as identification
- .slave property added to back-refference target module/slot    
*/

/*
	Configuration settings documentation:
	
	useMode (Integer) – Possible values are 1 and 2 as follows:
		1 – The calendar's HTML will be directly appended to the field supplied by target
		2 – The calendar will appear as a popup when the field with the id supplied in target is clicked.
	
	target (String) – The id of the field to attach the calendar to , usually a text input field when using useMode 2.
	
	isStripped (Boolean) – When set to true the calendar appears without the visual design - usually used with useMode 1
	
	selectedDate (Object) – When supplied , this object tells the calendar to open up with this date selected already.
	
	yearsRange (Array) – When supplied , this array sets the limits for the years enabled in the calendar.
	
	limitToToday (Boolean) – Enables you to limit the possible picking days to today's date.
	
	cellColorScheme (String) – Enables you to swap the colors of the date's cells from a wide range of colors.
		Available color schemes: torqoise,purple,pink,orange,peppermint,aqua,armygreen,bananasplit,beige,
		deepblue,greenish,lightgreen,  ocean_blue <-default
	
	dateFormat (String) - Enables you to easily switch the date format without any hassle at all! 
		Should you not supply anything this field will default to: "%m-%d-%Y"
		
		Possible values to use in the date format:
		
		%d - Day of the month, 2 digits with leading zeros
		%j - Day of the month without leading zeros
		
		%m - Numeric representation of a month, with leading zeros
		%M - A short textual representation of a month, three letters
		%n - Numeric representation of a month, without leading zeros
		%F - A full textual representation of a month, such as January or March
		
		%Y - A full numeric representation of a year, 4 digits
		%y - A two digit representation of a year
		
		You can of course put whatever divider you want between them.
		
	weekStartDay (Integer) : Enables you to change the day that the week starts on.
		Possible values 0 (Sunday) through 6 (Saturday)
		Default value is 1 (Monday)
		
	Note: We have implemented a way to change the image path of the img folder should you decide you want to move it somewhere else.
	Please read through the instructions on how to carefully accomplish that just in the next comment!
	
	Thanks for using my calendar !
	Itamar :-)
	
	itamar.arjuan@gmail.com
	
*/
// The language array - change these values to your language to better fit your needs!
g_l = [];
g_l["MONTHS"] = ["Janaury","February","March","April","May","June","July","August","September","October","November","December"];
g_l["DAYS_3"] = ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"];
g_l["MONTH_FWD"] = "Move a month forward";
g_l["MONTH_BCK"] = "Move a month backward";
g_l["YEAR_FWD"] = "Move a year forward";
g_l["YEAR_BCK"] = "Move a year backward";
g_l["CLOSE"] = "Close the calendar";
g_l["ERROR_2"] = g_l["ERROR_1"] = "Date object invalid!";
g_l["ERROR_4"] = g_l["ERROR_3"] = "Target invalid";

/* Changing the image path: WARNING! */
/*
	The image path can be changed easily , however a few important
	safety steps must take place!
	
	CSS as a rule-of-thumb is always looking for relative image paths to where the CSS
	file resides. Meaning , if we place the css document of JsDatePick somewhere else
	Since some of the elements inside the CSS have  background:url(img/someimage.png);
	
	The system will try to look for a file under the same folder where the CSS file is.
	So pay careful attention when moving the CSS file somewhere else as the images folder
	must be relative to it. If you want to put the CSS document somewhere else and the images somewhere
	else - you HAVE to look and replace each background:url(img/someimage.png); to the new path you desire.
	
	That way you ensure risk free operation of images.
	For any further questions or support about this issue - please consider the feedback form
	at javascriptcalendar.org
	Thank you!
*/
g_jsDatePickImagePath = "gfx/calendar/";
g_jsDatePickDirectionality = "ltr";

g_arrayOfUsedJsDatePickCalsGlobalNumbers = [];
g_arrayOfUsedJsDatePickCals = [];
g_currentDateObject = {};
g_currentDateObject.dateObject = new Date();

g_currentDateObject.day = g_currentDateObject.dateObject.getDate();
g_currentDateObject.month = g_currentDateObject.dateObject.getMonth() + 1;
g_currentDateObject.year = g_currentDateObject.dateObject.getFullYear();

JsgetElem = function(id){ return document.getElementById(id); };

String.prototype.trim = function() {
	return this.replace(/^\s+|\s+$/g,"");
};
String.prototype.ltrim = function() {
	return this.replace(/^\s+/,"");
};
String.prototype.rtrim = function() {
	return this.replace(/\s+$/,"");
};
String.prototype.strpad=function(){
	return (!isNaN(this) && this.toString().length==1)?"0"+this:this;
};

JsDatePick = function(configurationObject){
	if (document.all){
		this.isie = true;
		this.iever = JsDatePick.getInternetExplorerVersion();
	} else {
		this.isie = false;
	}
	
	this.oConfiguration = {};
	this.oCurrentDay = g_currentDateObject;
	this.monthsTextualRepresentation = g_l["MONTHS"];
	
	this.lastPostedDay = null;
	
	this.initialZIndex = 2;
	
	this.globalNumber = this.getUnUsedGlobalNumber();
	g_arrayOfUsedJsDatePickCals[this.globalNumber] = this;
	
	this.setConfiguration(configurationObject);
	this.makeCalendar();
};

JsDatePick.getCalInstanceById=function(id){ return g_arrayOfUsedJsDatePickCals[parseInt(id,10)]; };

JsDatePick.getInternetExplorerVersion=function(){
	var rv = -1, ua, re;
	if (navigator.appName == 'Microsoft Internet Explorer'){
		ua = navigator.userAgent;
		re = new RegExp("MSIE ([0-9]{1,}[\.0-9]{0,})");
		if (re.exec(ua) != null){
		  rv = parseFloat( RegExp.$1 );
		}
		return rv;
	}
};

JsDatePick.prototype.setC = function(obj, aClassName){
	if (this.isie && this.iever > 7){
		obj.setAttribute("class", aClassName);
	} else {
		obj.className = aClassName;
	}
};

JsDatePick.prototype.getUnUsedGlobalNumber = function(){
	
	var aNum = Math.floor(Math.random()*1000);
	
	while ( ! this.isUnique_GlobalNumber(aNum) ){
		aNum = Math.floor(Math.random()*1000);
	}
	
	return aNum;
};

JsDatePick.prototype.isUnique_GlobalNumber = function(aNum){
	var i;
	for (i=0; i<g_arrayOfUsedJsDatePickCalsGlobalNumbers.length; i++){
		if (g_arrayOfUsedJsDatePickCalsGlobalNumbers[i] == aNum){
			return false;
		}
	}
	return true;
};

JsDatePick.prototype.addOnSelectedDelegate = function(aDelegatedFunction){
	if (typeof(aDelegatedFunction) == "function"){
		this.addonSelectedDelegate = aDelegatedFunction;
	}
	return false;
};

JsDatePick.prototype.setOnSelectedDelegate = function(aDelegatedFunction){
	if (typeof(aDelegatedFunction) == "function"){
		this.onSelectedDelegate = aDelegatedFunction;
		return true;
	}
	return false;
};

JsDatePick.prototype.executeOnSelectedDelegateIfExists = function(){
	if (typeof(this.onSelectedDelegate) == "function"){
		this.onSelectedDelegate(this); // M.i.X !!!!!!
	}
	if (typeof(this.addonSelectedDelegate) == "function"){
		this.addonSelectedDelegate();
	}
};

JsDatePick.prototype.setRepopulationDelegate = function(aDelegatedFunction){
	if (typeof(aDelegatedFunction) == "function"){
		this.repopulationDelegate = aDelegatedFunction;
		return true;
	}
	return false;
};

JsDatePick.prototype.setConfiguration = function(aConf){ 
	this.oConfiguration.isStripped 		= (aConf["isStripped"] != null) ? aConf["isStripped"] : false;
	this.oConfiguration.useMode    		= (aConf["useMode"] != null) ? aConf["useMode"] : 1;
	this.oConfiguration.selectedDate   	= (aConf["selectedDate"] != null) ? aConf["selectedDate"] : null;
	this.oConfiguration.target			= (aConf["target"] != null) ? aConf["target"] : null;
	this.oConfiguration.yearsRange		= (aConf["yearsRange"] != null) ? aConf["yearsRange"] : [1971,2100];
	this.oConfiguration.limitToToday	= (aConf["limitToToday"] != null) ? aConf["limitToToday"] : false;
	this.oConfiguration.field			= (aConf["field"] != null) ? aConf["field"] : false;
	this.oConfiguration.cellColorScheme = (aConf["cellColorScheme"] != null) ? aConf["cellColorScheme"] : "ocean_blue";
	this.oConfiguration.dateFormat		= (aConf["dateFormat"] != null) ? aConf["dateFormat"] : "%m-%d-%Y";
	this.oConfiguration.imgPath			= (g_jsDatePickImagePath.length != null) ? g_jsDatePickImagePath : "gfx/calendar/";
	this.oConfiguration.weekStartDay   	= (aConf["weekStartDay"] != null) ? aConf["weekStartDay"] : 1;
	
	this.selectedDayObject = {};
	this.flag_DayMarkedBeforeRepopulation = false;
	this.flag_aDayWasSelected = false;
	this.lastMarkedDayObject = null;
	
	if (!this.oConfiguration.selectedDate){
		this.currentYear 	= this.oCurrentDay.year;
		this.currentMonth	= this.oCurrentDay.month;
		this.currentDay		= this.oCurrentDay.day;
	    }
        else // M.i.X!!!!
        {                   
        this.currentYear     = this.oConfiguration.selectedDate.year;
        this.currentMonth    = this.oConfiguration.selectedDate.month;
        this.currentDay        = this.oConfiguration.selectedDate.day;
        }
    
    
};

JsDatePick.prototype.resizeCalendar = function(){
	this.leftWallStrechedElement.style.height = "0px";
	this.rightWallStrechedElement.style.height = "0px";
	
	var totalHeight = this.JsDatePickBox.offsetHeight, newStrechedHeight = totalHeight-16;	
	
	if (newStrechedHeight < 0){
		return;
	}
	
	this.leftWallStrechedElement.style.height = newStrechedHeight+"px";
	this.rightWallStrechedElement.style.height = newStrechedHeight+"px";
	return true;
};

JsDatePick.prototype.closeCalendar = function(){
	this.JsDatePickBox.style.display = "none";
	document.onclick = function(){};
};

JsDatePick.prototype.populateFieldWithSelectedDate = function(){
	JsgetElem(this.oConfiguration.target).value = this.getSelectedDayFormatted();
	if (this.lastPickedDateObject){
		delete(this.lastPickedDateObject);
	}
	this.lastPickedDateObject = {};
	this.lastPickedDateObject.day = this.selectedDayObject.day;
	this.lastPickedDateObject.month = this.selectedDayObject.month;
	this.lastPickedDateObject.year = this.selectedDayObject.year;
	
	this.closeCalendar();
};

JsDatePick.prototype.makeCalendar = function(){
	var d = document, JsDatePickBox, clearfix, closeButton,leftWall,rightWall,topWall,bottomWall,topCorner,bottomCorner,wall,inputElement,aSpan,aFunc;
	
	JsDatePickBox = d.createElement("div");
	clearfix		= d.createElement("div");
	closeButton		= d.createElement("div");
	
	this.setC(JsDatePickBox, "JsDatePickBox");
	this.setC(clearfix, "clearfix");
	this.setC(closeButton, "jsDatePickCloseButton");
	closeButton.setAttribute("globalNumber",this.globalNumber);
	
	closeButton.onmouseover = function(){
		var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["CLOSE"]);
		gRef.setC(this, "jsDatePickCloseButtonOver");
	};
	
	closeButton.onmouseout = function(){
		var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText('');
		gRef.setC(this, "jsDatePickCloseButton");
	};
	
	closeButton.onmousedown = function(){
		var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["CLOSE"]);
		gRef.setC(this, "jsDatePickCloseButtonDown");
	};
	
	closeButton.onmouseup = function(){
		var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText('');
		gRef.setC(this, "jsDatePickCloseButton");
		gRef.closeCalendar();
	};
	
	this.JsDatePickBox = JsDatePickBox;
	
	leftWall  	= d.createElement("div");
	rightWall 	= d.createElement("div");
	topWall		= d.createElement("div");
	bottomWall	= d.createElement("div");
	
	this.setC(topWall, "topWall");
	this.setC(bottomWall, "bottomWall");
	
	if (this.isie && this.iever == 6){
		bottomWall.style.bottom = "-2px";
	}
	
	topCorner	 = d.createElement("div");
	bottomCorner = d.createElement("div");
	wall		 = d.createElement("div");
	
	this.setC(topCorner, "leftTopCorner");
	this.setC(bottomCorner, "leftBottomCorner");
	this.setC(wall, "leftWall");
	
	this.leftWallStrechedElement = wall;
	this.leftWall  = leftWall;
	this.rightWall = rightWall;
	
	leftWall.appendChild(topCorner);
	leftWall.appendChild(wall);
	leftWall.appendChild(bottomCorner);
	
	topCorner	 = d.createElement("div");
	bottomCorner = d.createElement("div");
	wall		 = d.createElement("div");
	
	this.setC(topCorner, "rightTopCorner");
	this.setC(bottomCorner, "rightBottomCorner");
	this.setC(wall, "rightWall");
	
	this.rightWallStrechedElement = wall;
	
	rightWall.appendChild(topCorner);
	rightWall.appendChild(wall);
	rightWall.appendChild(bottomCorner);
	
	if (this.oConfiguration.isStripped){
		this.setC(leftWall, "hiddenBoxLeftWall");
		this.setC(rightWall, "hiddenBoxRightWall");				
	} else {
		this.setC(leftWall, "boxLeftWall");
		this.setC(rightWall, "boxRightWall");
	}
	
	JsDatePickBox.appendChild(leftWall);
	JsDatePickBox.appendChild(this.getDOMCalendarStripped());
	JsDatePickBox.appendChild(rightWall);
	JsDatePickBox.appendChild(clearfix);
	
	if (!this.oConfiguration.isStripped){
		JsDatePickBox.appendChild(closeButton);
		JsDatePickBox.appendChild(topWall);
		JsDatePickBox.appendChild(bottomWall);
	}
	
	if (this.oConfiguration.useMode == 2){
		if (this.oConfiguration.target != false){
			if (typeof(JsgetElem(this.oConfiguration.target)) != null){
				inputElement = JsgetElem(this.oConfiguration.target);
		
				aSpan = document.createElement("span");
				inputElement.parentNode.replaceChild(aSpan,inputElement);
				aSpan.appendChild(inputElement);
		
				inputElement.setAttribute("globalNumber",this.globalNumber);
				inputElement.onclick = function(){ JsDatePick.getCalInstanceById(this.getAttribute("globalNumber")).showCalendar(); };
				inputElement.onfocus = function(){ JsDatePick.getCalInstanceById(this.getAttribute("globalNumber")).showCalendar(); };
				
				aSpan.style.position = "relative";
				this.initialZIndex++;
				
				JsDatePickBox.style.zIndex = this.initialZIndex.toString();
				JsDatePickBox.style.position = "absolute";
				JsDatePickBox.style.top = "18px";
				JsDatePickBox.style.left = "0px";
				JsDatePickBox.style.display = "none";
				aSpan.appendChild(JsDatePickBox);
				
				aFunc = new Function("g_arrayOfUsedJsDatePickCals["+this.globalNumber+"].populateFieldWithSelectedDate();");
				
				this.setOnSelectedDelegate(aFunc);
			} else {
				alert(g_l["ERROR_3"]);
			}
		}
	} else {
		if (this.oConfiguration.target != null){
			JsgetElem(this.oConfiguration.target).appendChild(JsDatePickBox);
			JsgetElem(this.oConfiguration.target).style.position = "relative";
			JsDatePickBox.style.position = "absolute";
			JsDatePickBox.style.top = "0px";
			JsDatePickBox.style.left = "0px";
			this.resizeCalendar();
			this.executePopulationDelegateIfExists();
		} else {
			alert(g_l["ERROR_4"]);
		}
	}
};

JsDatePick.prototype.determineFieldDate = function(){
	var aField,divider,dateMold,array,array2,i,dI,yI,mI,tflag=false,fflag=false;
	
	if (this.lastPickedDateObject){
		this.setSelectedDay({
			year:parseInt(this.lastPickedDateObject.year),
			month:parseInt(this.lastPickedDateObject.month,10),
			day:parseInt(this.lastPickedDateObject.day,10)
		});
	} else {
		aField = JsgetElem(this.oConfiguration.target);
		
		if (aField.value.trim().length == 0){
			this.unsetSelection();
			
			if (typeof(this.oConfiguration.selectedDate) == "object" && this.oConfiguration.selectedDate){
				this.setSelectedDay({
					year:parseInt(this.oConfiguration.selectedDate.year),
					month:parseInt(this.oConfiguration.selectedDate.month,10),
					day:parseInt(this.oConfiguration.selectedDate.day,10)
				});
			}
			
		} else {
			if (aField.value.trim().length > 5){
				divider = this.senseDivider(this.oConfiguration.dateFormat);
				dateMold = this.oConfiguration.dateFormat;
				array 	= aField.value.trim().split(divider);
				array2 	= dateMold.trim().split(divider);
				i=dI=yI=mI=0;
				
				for (i=0; i<array2.length; i++){
					switch (array2[i]){
						case "%d": case "%j": dI = i; break;
						case "%m": case "%n": mI = i; break;
						case "%M": mI = i; tflag=true; break;
						case "%F": mI = i; fflag=true; break;
						case "%Y": case "%y": yI = i;
					}
				}
				
				if (tflag){
					for (i=0; i<12; i++){
						if (g_l["MONTHS"][i].substr(0,3).toUpperCase() == array[mI].toUpperCase()){
							mI = i+1; break;
						}
					}
				} else if (fflag){
					for (i=0; i<12; i++){
						if (g_l["MONTHS"][i].toLowerCase() == array[mI].toLowerCase()){
							mI = i+1; break;
						}
					}
				} else {
					mI = parseInt(array[mI],10);
				}
				
				this.setSelectedDay({
					year:parseInt(array[yI],10),
					month:mI,
					day:parseInt(array[dI],10)
				});
			} else {
				this.unsetSelection();
				return;
			}
		}
	}
};

JsDatePick.prototype.senseDivider=function(aStr){return aStr.replace("%d","").replace("%j","").replace("%m","").replace("%M","").replace("%n","").replace("%F","").replace("%Y","").replace("%y","").substr(0,1);};

JsDatePick.prototype.showCalendar = function(){
	if (this.JsDatePickBox.style.display == "none"){
		this.determineFieldDate();
		this.JsDatePickBox.style.display = "block";
		this.resizeCalendar();
		this.executePopulationDelegateIfExists();
		this.JsDatePickBox.onmouseover = function(){
			document.onclick = function(){};
		};
		
		this.JsDatePickBox.setAttribute("globalCalNumber", this.globalNumber);		
		this.JsDatePickBox.onmouseout = function(){
			document.onclick = new Function("g_arrayOfUsedJsDatePickCals["+this.getAttribute("globalCalNumber")+"].closeCalendar();");
		};
		
	} else {
		return;
	}
};

JsDatePick.prototype.isAvailable = function(y, m, d){
	if (y > this.oCurrentDay.year){
		return false;
	}
	
	if (m > this.oCurrentDay.month && y == this.oCurrentDay.year){
		return false;
	}
	
	if (d > this.oCurrentDay.day && m == this.oCurrentDay.month && y == this.oCurrentDay.year ){
		return false;
	}
	
	return true;
};

JsDatePick.prototype.getDOMCalendarStripped = function(){
	var d = document,boxMain,boxMainInner,clearfix,boxMainCellsContainer,tooltip,weekDaysRow,clearfix2;
	
	boxMain = d.createElement("div");
	if (this.oConfiguration.isStripped){
		this.setC(boxMain, "boxMainStripped");
	} else {
		this.setC(boxMain, "boxMain");
	}
	
	this.boxMain = boxMain;
	
	boxMainInner 			= d.createElement("div");
	clearfix	 			= d.createElement("div");
	boxMainCellsContainer 	= d.createElement("div");
	tooltip					= d.createElement("div");
	weekDaysRow				= d.createElement("div");
	clearfix2				= d.createElement("div");
	
	this.setC(clearfix, "clearfix");
	this.setC(clearfix2, "clearfix");
	this.setC(boxMainInner, "boxMainInner");
	this.setC(boxMainCellsContainer, "boxMainCellsContainer");
	this.setC(tooltip, "tooltip");
	this.setC(weekDaysRow, "weekDaysRow");
	
	this.tooltip = tooltip;
	
	boxMain.appendChild(boxMainInner);
	
	this.controlsBar = this.getDOMControlBar();
	this.makeDOMWeekDays(weekDaysRow);
	
	boxMainInner.appendChild(this.controlsBar);
	boxMainInner.appendChild(clearfix);
	boxMainInner.appendChild(tooltip);
	boxMainInner.appendChild(weekDaysRow);
	boxMainInner.appendChild(boxMainCellsContainer);
	boxMainInner.appendChild(clearfix2);
	
	this.boxMainCellsContainer = boxMainCellsContainer;
	this.populateMainBox(boxMainCellsContainer);
	
	return boxMain;
};

JsDatePick.prototype.makeDOMWeekDays = function(aWeekDaysRow){
	var i=0,d = document,weekDaysArray = g_l["DAYS_3"],textNode,weekDay;	
	
	for (i=this.oConfiguration.weekStartDay; i<7; i++){
		weekDay 	= d.createElement("div");
		textNode 	= d.createTextNode(weekDaysArray[i]);
		this.setC(weekDay, "weekDay");
		
		weekDay.appendChild(textNode);
		aWeekDaysRow.appendChild(weekDay);
	}
	
	if (this.oConfiguration.weekStartDay > 0){
		for (i=0; i<this.oConfiguration.weekStartDay; i++){
			weekDay 	= d.createElement("div");
			textNode 	= d.createTextNode(weekDaysArray[i]);
			this.setC(weekDay, "weekDay");
			
			weekDay.appendChild(textNode);
			aWeekDaysRow.appendChild(weekDay);
		}
	}
	weekDay.style.marginRight = "0px";
};

JsDatePick.prototype.repopulateMainBox = function(){
	while (this.boxMainCellsContainer.firstChild){
		this.boxMainCellsContainer.removeChild(this.boxMainCellsContainer.firstChild);
	}
	
	this.populateMainBox(this.boxMainCellsContainer);
	this.resizeCalendar();
	this.executePopulationDelegateIfExists();
};

JsDatePick.prototype.executePopulationDelegateIfExists = function(){
	if (typeof(this.repopulationDelegate) == "function"){
		this.repopulationDelegate();
	}
};

JsDatePick.prototype.populateMainBox = function(aMainBox){
	var d = document,aDayDiv,aTextNode,columnNumber = 1,disabledDayFlag = false,cmpMonth = this.currentMonth-1,oDay,iStamp,skipDays,i,currentColorScheme;
	
	oDay = new Date(this.currentYear, cmpMonth, 1,1,0,0);
	iStamp = oDay.getTime();
	
	this.flag_DayMarkedBeforeRepopulation = false;
	this.setControlBarText(this.monthsTextualRepresentation[cmpMonth] + ", " + this.currentYear);
	
	skipDays = parseInt(oDay.getDay())-this.oConfiguration.weekStartDay;
	if (skipDays < 0){
		skipDays = skipDays + 7;
	}
	
	i=0;
	for (i=0; i<skipDays; i++){
		aDayDiv = d.createElement("div");
		this.setC(aDayDiv, "skipDay");
		aMainBox.appendChild(aDayDiv);
		if (columnNumber == 7){
			columnNumber = 1;
		} else {
			columnNumber++;
		}
	}
	
	while (oDay.getMonth() == cmpMonth){
		disabledDayFlag = false;
		aDayDiv 	= d.createElement("div");
		
		if (this.lastPostedDay){
			if (this.lastPostedDay == oDay.getDate()){
				aTextNode	= parseInt(this.lastPostedDay,10)+1;
			} else {
				aTextNode	= d.createTextNode(oDay.getDate());
			}
		} else {
			aTextNode	= d.createTextNode(oDay.getDate());
		}
		
		aDayDiv.appendChild(aTextNode);
		aMainBox.appendChild(aDayDiv);
		
		aDayDiv.setAttribute("globalNumber",this.globalNumber);
		
		if (columnNumber == 7){
			if (g_jsDatePickDirectionality == "ltr"){
				aDayDiv.style.marginRight = "0px";
			} else {
				aDayDiv.style.marginLeft = "0px";
			}
		}
		
		if (this.isToday(oDay)){
			aDayDiv.setAttribute("isToday",1);
		}
		
		if (this.oConfiguration.limitToToday){
			if ( ! this.isAvailable(this.currentYear, this.currentMonth, parseInt(oDay.getDate()) ) ){
				disabledDayFlag = true;
				aDayDiv.setAttribute("isJsDatePickDisabled",1);
			}
		}

		aDayDiv.onmouseover = function(){
			var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber")),currentColorScheme;
			currentColorScheme = gRef.getCurrentColorScheme();
			
			if (parseInt(this.getAttribute("isSelected")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isToday")) == 1){
				gRef.setC(this, "dayOverToday");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayOver.gif) left top no-repeat";
			} else {
				gRef.setC(this, "dayOver");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayOver.gif) left top no-repeat";
			}
		};
		
		aDayDiv.onmouseout = function(){
			var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber")),currentColorScheme;
			currentColorScheme = gRef.getCurrentColorScheme();
			
			if (parseInt(this.getAttribute("isSelected")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isToday")) == 1){
				gRef.setC(this, "dayNormalToday");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayNormal.gif) left top no-repeat";
			} else {
				gRef.setC(this, "dayNormal");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayNormal.gif) left top no-repeat";
			}
		};
		
		aDayDiv.onmousedown = function(){
			var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber")),currentColorScheme;
			currentColorScheme = gRef.getCurrentColorScheme();
			
			if (parseInt(this.getAttribute("isSelected")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isToday")) == 1){
				gRef.setC(this, "dayDownToday");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayDown.gif) left top no-repeat";
			} else {
				gRef.setC(this, "dayDown");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayDown.gif) left top no-repeat";
			}
		};
		
		aDayDiv.onmouseup = function(){
			var gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber")),currentColorScheme;
			currentColorScheme = gRef.getCurrentColorScheme();
			
			if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
				return;
			}
			if (parseInt(this.getAttribute("isToday")) == 1){
				gRef.setC(this, "dayNormalToday");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayNormal.gif) left top no-repeat";
			} else {
				gRef.setC(this, "dayNormal");
				this.style.background = "url(" + gRef.oConfiguration.imgPath + currentColorScheme + "_dayNormal.gif) left top no-repeat";
			}
			
			gRef.setDaySelection(this);
			gRef.executeOnSelectedDelegateIfExists();
		};
	
		if (this.isSelectedDay(oDay.getDate())){
			aDayDiv.setAttribute("isSelected",1);
			this.flag_DayMarkedBeforeRepopulation = true;
			this.lastMarkedDayObject = aDayDiv;
			
			if (parseInt(aDayDiv.getAttribute("isToday")) == 1){
				this.setC(aDayDiv, "dayDownToday");
				aDayDiv.style.background = "url(" + this.oConfiguration.imgPath + this.oConfiguration.cellColorScheme + "_dayDown.gif) left top no-repeat";
			} else {
				this.setC(aDayDiv, "dayDown");
				aDayDiv.style.background = "url(" + this.oConfiguration.imgPath + this.oConfiguration.cellColorScheme + "_dayDown.gif) left top no-repeat";
			}	
			
		} else {
			currentColorScheme = this.getCurrentColorScheme();
			
			if (parseInt(aDayDiv.getAttribute("isToday")) == 1){
				if (disabledDayFlag){
					this.setC(aDayDiv, "dayDisabled");
					aDayDiv.style.background = "url(" + this.oConfiguration.imgPath + this.oConfiguration.cellColorScheme + "_dayNormal.gif) left top no-repeat";
				} else {
					this.setC(aDayDiv, "dayNormalToday");
					aDayDiv.style.background = "url(" + this.oConfiguration.imgPath + this.oConfiguration.cellColorScheme + "_dayNormal.gif) left top no-repeat";
				}
			} else {
				if (disabledDayFlag){
					this.setC(aDayDiv, "dayDisabled");
					aDayDiv.style.background = "url(" + this.oConfiguration.imgPath + this.oConfiguration.cellColorScheme + "_dayNormal.gif) left top no-repeat";
				} else {
					this.setC(aDayDiv, "dayNormal");
					aDayDiv.style.background = "url(" + this.oConfiguration.imgPath + this.oConfiguration.cellColorScheme + "_dayNormal.gif) left top no-repeat";
				}
			}
		}
		
		if (columnNumber == 7){
			columnNumber = 1;
		} else {
			columnNumber++;
		}
		iStamp += 86400000;
		oDay.setTime(iStamp);
	}
	
	this.lastPostedDay = null;
	
	return aMainBox;
};

JsDatePick.prototype.unsetSelection = function(){
	this.flag_aDayWasSelected = false;
	this.selectedDayObject = {};
	this.repopulateMainBox();
};

JsDatePick.prototype.setSelectedDay = function(dateObject){
	this.flag_aDayWasSelected = true;
	
	this.selectedDayObject.day = parseInt(dateObject.day,10);
	this.selectedDayObject.month = parseInt(dateObject.month,10);
	this.selectedDayObject.year = parseInt(dateObject.year);
	
	this.currentMonth 	= dateObject.month;
	this.currentYear	= dateObject.year;
	
	this.repopulateMainBox();
};

JsDatePick.prototype.isSelectedDay = function(aDate){
	if (this.flag_aDayWasSelected){
		if (parseInt(aDate) == this.selectedDayObject.day &&
			this.currentMonth == this.selectedDayObject.month &&
			this.currentYear == this.selectedDayObject.year){
			return true;
		} else {
			return false;
		}
	}
	return false;
};

JsDatePick.prototype.getSelectedDay = function(){
	if (this.flag_aDayWasSelected){
		return this.selectedDayObject;
	} else {
		return false;
	}
};

JsDatePick.prototype.getSelectedDayFormatted = function(){
	if (this.flag_aDayWasSelected){
		
		var dateStr = this.oConfiguration.dateFormat;
		
		dateStr = dateStr.replace("%d", this.selectedDayObject.day.toString().strpad());
		dateStr = dateStr.replace("%j", this.selectedDayObject.day);
		
		dateStr = dateStr.replace("%m", this.selectedDayObject.month.toString().strpad());
		dateStr = dateStr.replace("%M", g_l["MONTHS"][this.selectedDayObject.month-1].substr(0,3).toUpperCase());
		dateStr = dateStr.replace("%n", this.selectedDayObject.month);
		dateStr = dateStr.replace("%F", g_l["MONTHS"][this.selectedDayObject.month-1]);
		
		dateStr = dateStr.replace("%Y", this.selectedDayObject.year);
		dateStr = dateStr.replace("%y", this.selectedDayObject.year.toString().substr(2,2));
		
		return dateStr;
	} else {
		return false;
	}
};

JsDatePick.prototype.setDaySelection = function(anElement){
	var currentColorScheme = this.getCurrentColorScheme();
	
	if  (this.flag_DayMarkedBeforeRepopulation){
		/* Un mark last selected day */
		this.lastMarkedDayObject.setAttribute("isSelected",0);
		
		if (parseInt(this.lastMarkedDayObject.getAttribute("isToday")) == 1){
			this.setC(this.lastMarkedDayObject, "dayNormalToday");
			this.lastMarkedDayObject.style.background = "url(" + this.oConfiguration.imgPath + currentColorScheme + "_dayNormal.gif) left top no-repeat";
		} else {
			this.setC(this.lastMarkedDayObject, "dayNormal");
			this.lastMarkedDayObject.style.background = "url(" + this.oConfiguration.imgPath + currentColorScheme + "_dayNormal.gif) left top no-repeat";
		}
	}
	
	this.flag_aDayWasSelected = true;
	this.selectedDayObject.year  = this.currentYear;
	this.selectedDayObject.month = this.currentMonth;
	this.selectedDayObject.day   = parseInt(anElement.innerHTML);
	
	this.flag_DayMarkedBeforeRepopulation = true;
	this.lastMarkedDayObject = anElement;
	
	anElement.setAttribute("isSelected",1);
	
	if (parseInt(anElement.getAttribute("isToday")) == 1){
		this.setC(anElement, "dayDownToday");
		anElement.style.background = "url(" + this.oConfiguration.imgPath + currentColorScheme + "_dayDown.gif) left top no-repeat";
	} else {
		this.setC(anElement, "dayDown");
		anElement.style.background = "url(" + this.oConfiguration.imgPath + currentColorScheme + "_dayDown.gif) left top no-repeat";
	}
};

JsDatePick.prototype.isToday = function(aDateObject){
	var cmpMonth = this.oCurrentDay.month-1;
	if (aDateObject.getDate() == this.oCurrentDay.day &&
		aDateObject.getMonth() == cmpMonth &&
		aDateObject.getFullYear() == this.oCurrentDay.year){
		return true;
	}
	return false;
};

JsDatePick.prototype.setControlBarText = function(aText){
	var aTextNode = document.createTextNode(aText);
	
	while (this.controlsBarTextCell.firstChild){
		this.controlsBarTextCell.removeChild(this.controlsBarTextCell.firstChild);
	}
	
	this.controlsBarTextCell.appendChild(aTextNode);
};

JsDatePick.prototype.setTooltipText = function(aText){
	while (this.tooltip.firstChild){
		this.tooltip.removeChild(this.tooltip.firstChild);
	}
	
	var aTextNode = document.createTextNode(aText);
	this.tooltip.appendChild(aTextNode);
};

JsDatePick.prototype.moveForwardOneYear = function(){
	var desiredYear = this.currentYear + 1;
	if (desiredYear < parseInt(this.oConfiguration.yearsRange[1])){
		this.currentYear++;
		this.repopulateMainBox();
		return true;
	} else {
		return false;
	}
};

JsDatePick.prototype.moveBackOneYear = function(){
	var desiredYear = this.currentYear - 1;
	
	if (desiredYear > parseInt(this.oConfiguration.yearsRange[0])){
		this.currentYear--;
		this.repopulateMainBox();
		return true;
	} else {
		return false;
	}
};

JsDatePick.prototype.moveForwardOneMonth = function(){
	
	if (this.currentMonth < 12){
		this.currentMonth++;
	} else {
		if (this.moveForwardOneYear()){
			this.currentMonth = 1;
		} else {
			this.currentMonth = 12;
		}
	}
	
	this.repopulateMainBox();
};

JsDatePick.prototype.moveBackOneMonth = function(){
	
	if (this.currentMonth > 1){
		this.currentMonth--;
	} else {
		if (this.moveBackOneYear()){
			this.currentMonth = 12;
		} else {
			this.currentMonth = 1;
		}
	}
	
	this.repopulateMainBox();
};

JsDatePick.prototype.getCurrentColorScheme = function(){
	return this.oConfiguration.cellColorScheme;
};

JsDatePick.prototype.getDOMControlBar = function(){
	var d = document, controlsBar,monthForwardButton,monthBackwardButton,yearForwardButton,yearBackwardButton,controlsBarText;
	
	controlsBar 			= d.createElement("div");
	monthForwardButton		= d.createElement("div");
	monthBackwardButton		= d.createElement("div");
	yearForwardButton		= d.createElement("div");
	yearBackwardButton		= d.createElement("div");
	controlsBarText			= d.createElement("div");
	
	this.setC(controlsBar, "controlsBar");
	this.setC(monthForwardButton, "monthForwardButton");
	this.setC(monthBackwardButton, "monthBackwardButton");
	this.setC(yearForwardButton, "yearForwardButton");
	this.setC(yearBackwardButton, "yearBackwardButton");
	this.setC(controlsBarText, "controlsBarText");
		
	controlsBar.setAttribute("globalNumber",this.globalNumber);
	monthForwardButton.setAttribute("globalNumber",this.globalNumber);
	monthBackwardButton.setAttribute("globalNumber",this.globalNumber);
	yearBackwardButton.setAttribute("globalNumber",this.globalNumber);
	yearForwardButton.setAttribute("globalNumber",this.globalNumber);
	
	this.controlsBarTextCell = controlsBarText;
	
	controlsBar.appendChild(monthForwardButton);
	controlsBar.appendChild(monthBackwardButton);
	controlsBar.appendChild(yearForwardButton);
	controlsBar.appendChild(yearBackwardButton);
	controlsBar.appendChild(controlsBarText);
	
	monthForwardButton.onmouseover = function(){
		var	gRef,parentElement;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["MONTH_FWD"]);
		gRef.setC(this, "monthForwardButtonOver");
	};
	
	monthForwardButton.onmouseout = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText('');
		gRef.setC(this, "monthForwardButton");
	};
	
	monthForwardButton.onmousedown = function(){
		var gRef,parentElement;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}		
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["MONTH_FWD"]);
		gRef.setC(this, "monthForwardButtonDown");
	};
	
	monthForwardButton.onmouseup = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["MONTH_FWD"]);
		gRef.setC(this, "monthForwardButton");
		gRef.moveForwardOneMonth();
	};
	
	/* Month backward button event handlers */
	
	monthBackwardButton.onmouseover = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["MONTH_BCK"]);
		gRef.setC(this, "monthBackwardButtonOver");
	};
	
	monthBackwardButton.onmouseout = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText('');
		gRef.setC(this, "monthBackwardButton");
	};
	
	monthBackwardButton.onmousedown = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["MONTH_BCK"]);
		gRef.setC(this, "monthBackwardButtonDown");
	};
	
	monthBackwardButton.onmouseup = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["MONTH_BCK"]);
		gRef.setC(this, "monthBackwardButton");
		gRef.moveBackOneMonth();
	};
	
	/* Year forward button */
	
	yearForwardButton.onmouseover = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;		
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["YEAR_FWD"]);
		gRef.setC(this, "yearForwardButtonOver");
	};
	
	yearForwardButton.onmouseout = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;			
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText('');
		gRef.setC(this, "yearForwardButton");
	};
	
	yearForwardButton.onmousedown = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["YEAR_FWD"]);
		gRef.setC(this, "yearForwardButtonDown");
	};
	
	yearForwardButton.onmouseup = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["YEAR_FWD"]);
		gRef.setC(this, "yearForwardButton");
		gRef.moveForwardOneYear();
	};
	
	/* Year backward button */
	
	yearBackwardButton.onmouseover = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["YEAR_BCK"]);
		gRef.setC(this, "yearBackwardButtonOver");
	};
	
	yearBackwardButton.onmouseout = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}		
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText('');
		gRef.setC(this, "yearBackwardButton");
	};
	
	yearBackwardButton.onmousedown = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["YEAR_BCK"]);
		gRef.setC(this, "yearBackwardButtonDown");
	};
	
	yearBackwardButton.onmouseup = function(){
		var parentElement,gRef;
		if (parseInt(this.getAttribute("isJsDatePickDisabled")) == 1){
			return;
		}
		parentElement = this.parentNode;
		while (parentElement.className != "controlsBar"){
			parentElement = parentElement.parentNode;
		}		
		gRef = JsDatePick.getCalInstanceById(this.getAttribute("globalNumber"));
		gRef.setTooltipText(g_l["YEAR_BCK"]);
		gRef.setC(this, "yearBackwardButton");
		gRef.moveBackOneYear();
	};
	
	return controlsBar;
};