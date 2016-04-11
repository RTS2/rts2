<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:param name='debug'>0</xsl:param>
<xsl:output method='text' indent='no'/>

<xsl:variable name='printd'>
<xsl:choose>
  <xsl:when test='$debug = 0'>
# </xsl:when>
  <xsl:otherwise>
echo `date +'%D %T.%02N'` </xsl:otherwise>
</xsl:choose>  
</xsl:variable>

<xsl:template match='/'>

if ( ! (${?imgid}) ) @ imgid = 1

if ( ! (${?lastoffimage}) ) set lastoffimage=-1

if ( ! (${?last_offtarget}) ) set last_offtarget=-1

if ( ! (${?gdiff}) ) set gdiff=0
	
# next autoguider attempt
if ( ! (${?nextautog}) ) set nextautog=`date +%s`

if ( ! (${?guideon}) ) set guideon=`date +%s`

set continue=1
unset imgdir
set xpa=0
# XPA takes way too long to queury. Uncomment this if you would like to check for it anyway, but expect ~0.2 sec penalty
# xpaget ds9 >&amp; /dev/null
# if ( $? == 0 ) set xpa=1

set defoc_toffs=0

set lastamp=0

if ( ! (${?autog}) ) set autog='UNKNOWN'

<xsl:copy-of select='$printd'/> "script running"

<xsl:apply-templates select='*'/>

</xsl:template>

<xsl:variable name='abort'>
if ( -e $rts2abort ) then
	rts2-logcom 'Aborting observations. Please wait'
	source $RTS2/bin/.rts2-runabort
	if ( $autog == "ON" ) then
		rts2-logcom 'Switching off guiding'
		tele autog OFF
	endif
	rm -f $lasttarget
	exit
endif
if ( $ignoreday == 0 ) then
	set ms=`$xmlrpc --master-state rnight`
	if ( $? != 0 || $ms == 0 ) then
		rm -f $lasttarget
		set continue=0
	endif
endif
set in=`$xmlrpc -G SEL.interrupt`
if ( $? == 0 &amp;&amp; $in == 1 ) then
	rm -f $lasttarget
	set continue=0
	rts2-logcom "Interrupt target $name ($tar_id)"
        set weather_reason=`$xmlrpc -G .weather_reason`
        if ( "$weather_reason" != "" ) then
            rts2-logcom $weather_reason
        else
            rts2-logcom "No weather reason for interrupt."
        endif
        if ( "$new_guide" != "OFF" ) then
            rts2-logcom "Switch guider from $new_guide to OFF"
	    tele autog OFF
            set new_guide="OFF"
        endif
endif
</xsl:variable>

<!-- grab guider image -->
<xsl:variable name="grabguide">
	rts2-logcom "Grabbing autoguider image"
	<!-- tele grab -->
	set lllastgrab = `ls -rt /Realtime/guider/frames/0*.fits | tail -1`
	set dir=/Realtime/guider/frames/ROBOT_`rts2-state -e %N`
	mkdir -p $dir
	set lastgrab=`rts2-image -p "${dir}/${name}_%03u.fits" $lllastgrab`
	mv $lllastgrab $lastgrab
</xsl:variable>

<!-- called after guider is set to ON. Wait for some time for guider to settle down,
  and either find guide star, or give up-->
<xsl:variable name='guidetest'>
	tele ggain On
	tele glev 100
	tele autog ON
	@ retr = 5
#     Ted says 10 is too short to acquire guide star, go to 30
#	sleep 10
	sleep 30
	while ( $retr &gt;= 0 )
		set gs=`tele autog ?`
		if ( $gs == 'ON' ) then
			break
		endif
		$xmlrpc --quiet -c TELE.info
		set fwhms=`$xmlrpc --quiet -G TELE.fwhms`
		set guide_seg=`$xmlrpc --quiet -G TELE.guide_seg`
		set isguiding=`$xmlrpc --quiet -G TELE.isguiding`
		rts2-logcom "Autoguider started, but star still not acquired. FWHMS $fwhms SEG $guide_seg isguiding $isguiding"
		@ retr --
		sleep 20
	end
	@ nextautog = $nowdate + 7200
	@ guideon = $nowdate + 60
	set guide_seg=`$xmlrpc --quiet -G TELE.guide_seg`
	if ( $retr &gt; 0 ) then
		rts2-logcom "Successfully switched autoguider to ON on segment $guide_seg"
	else
		set textdate = `awk 'BEGIN { print strftime("%T"'",$nextautog); }"`
		rts2-logcom "Autoguider command failed, try again at $textdate; segment $guide_seg"
	endif
	<xsl:copy-of select="$grabguide"/>
	rts2-logcom "Current guider image saved in $lastgrab"
</xsl:variable>

<xsl:template match="disable">
$RTS2/bin/rts2-target -d $tar_id
</xsl:template>

<xsl:template match="tempdisable">
$RTS2/bin/rts2-target -n +<xsl:value-of select='.'/> $tar_id
</xsl:template>

<xsl:template match="exposure">
if ( $continue == 1 ) then
  	<xsl:copy-of select='$printd'/> "starting pre-exposure checks"
	set last_obs_id=`$xmlrpc --quiet -G IMGP.obs_id`
        set cname=`$xmlrpc --quiet -G IMGP.object`
	set ora_l=`$xmlrpc --quiet -G IMGP.ora`
	set odec_l=`$xmlrpc --quiet -G IMGP.odec`
	set ora=`printf "%.0f" $ora_l`
	set odec=`printf "%.0f" $odec_l`
	if ( $cname == $name ) then
		if ( ${%ora} &gt; 0 &amp;&amp; ${%odec} &gt; 0 &amp;&amp; $ora &gt; -800 &amp;&amp; $ora &lt; 800 &amp;&amp; $odec &gt; -700 &amp;&amp; $odec &lt; 700 ) then
			set apply=`$xmlrpc --quiet -G IMGP.apply_corrections`
			set imgnum=`$xmlrpc --quiet -G IMGP.img_num`

			if ( $apply == 0 ) then
        			rts2-logcom "Corrections disabled, do not apply correction $ora_l" $odec_l""	
			else

				set xoffs=`$xmlrpc --quiet -G IMGP.xoffs`
				set yoffs=`$xmlrpc --quiet -G IMGP.yoffs`

				$xmlrpc --quiet -c TELE.info
				set currg=`$xmlrpc --quiet -G TELE.guider_working`

				if ( $currg != '1' ) then
					if ( $imgnum &gt; $lastoffimage ) then
						if ( $tar_id == $last_offtarget ) then
							$xmlrpc -s TELE.CORR_ -- "${ora_l}s ${odec_l}s"
							$RTS2/bin/jt '/api/cmd?d=TELE&amp;c=info'
							set toffs=`$xmlrpc -G TELE.total_offsets`
							set rra=`echo "$toffs" | awk '{printf "%d",$1 * -3600.0;}'`
							set rdec=`echo "$toffs" | awk '{printf "%d",$2 * -3600.0;}'`
							<xsl:copy-of select='$printd'/> "Total offsets $toffs ($rra $rdec)"
							if ( $rra &gt; -1 &amp;&amp; $rra &lt; 1 ) then
								set rra = 0
							endif
							if ( $rdec &gt; -1 &amp;&amp; $rdec &lt; 1 ) then
								set rdec = 0
							endif
							if ( $rra != 0 || $rdec != 0 ) then
								set rra_l=`echo "$toffs" | awk '{printf "%f",$1 * -3600.0;}'`
								set rdec_l=`echo "$toffs" | awk '{printf "%f",$2 * -3600.0;}'`
								<xsl:copy-of select='$printd'/> "Offseting $rra $rdec ($ora_l $odec_l; $xoffs $yoffs) img_num $imgnum autog $autog"
								rts2-logcom "Applying offsets from catalogue match" `echo $rra_l $rdec_l | awk '{ printf "%+0.2f\" %+0.2f\"",$1,$2; }'`
								tele offset $rra $rdec
								@ lastoffimage = $imgnum
							<xsl:if test='$debug != 0'>
							else
								rts2-logcom "Zero offsets, not offseting ($rra $rdec; $ora_l $odec_l; $xoffs $yoffs) img_num $imgnum autog $autog"
							</xsl:if>
							endif
							<!--if ( ${?lastimage} ) then
								set lastoffimage=`echo $lastimage | sed 's#.*/\([0-9][0-9][0-9][0-9]\).*#\1#'`
								<xsl:copy-of select='$printd'/> "setting last $lastoffimage"
							endif -->
						else
							<xsl:copy-of select='$printd'/> "applying first offsets ($ora_l $odec_l; $xoffs $yoffs) $tar_id $last_offtarget"
							$xmlrpc -s TELE.OFFS -- "${ora_l}s ${odec_l}s"
							$xmlrpc -s TELE.CORR_ -- "${ora_l}s ${odec_l}s"
							set last_offtarget = $tar_id
							@ lastoffimage = $imgnum
						endif
					<xsl:if test='$debug != 0'>
					else
						rts2-logcom "Older or same image received - not offseting $ora $odec ($ora_l $odec_l; $xoffs $yoffs) img_num $imgnum lastimage $lastoffimage"
					</xsl:if>	
					endif
				<xsl:if test='$debug != 0'>
				else
					rts2-logcom "Autoguider is $currg - not offseting $ora $odec ($ora_l $odec_l; $xoffs $yoffs) img_num $imgnum"
				</xsl:if>	
				endif
			endif
		else
			rts2-logcom "Too big offset " `echo $ora_l $odec_l | awk '{ printf "%+0.2f\" %+0.2f\"",$1,$2; }'`
		endif	  	
	<xsl:if test='$debug != 0'>
	else
		rts2-logcom "Not offseting - correction from different target (observing $name, correction from $cname)"
	</xsl:if>	
	endif
	set diff_l=`echo $defoc_toffs - $defoc_current | bc`
	if ( $diff_l != 0 ) then
		set diff=`printf '%+0f' $diff_l`
		set diff_f=`printf '%+02f' $diff`
	        tele hfocus $diff_f
	        if ( $new_guide == "ON" ) then
	           set gdiff=`echo $diff_f | awk '{ printf "%+i",$1*(-4313); }'`
	           rts2-logcom "Offset tel focus $diff_f ( $defoc_toffs - $defoc_current ), guide focus $gdiff"
		   tele gfoc $gdiff
	        else
	           rts2-logcom "Offset tel focus $diff_f ( $defoc_toffs - $defoc_current ), guide OFF, no guide focus offset"
	        endif
		set defoc_current=`echo $defoc_current + $diff_l | bc`
	<xsl:if test='$debug != 0'>
	else
		rts2-logcom "Keeping tel focus offset ( $defoc_toffs - $defoc_current )"
	</xsl:if>
	endif

	if ( $autog == 'ON' ) then
		$xmlrpc --quiet -c TELE.info
		set guidestatus=`$xmlrpc --quiet -G TELE.guider_working`
		if ( $guidestatus == 1 ) then
			set guidestatus = "ON"
		else
			set guidestatus = "OFF"
		endif
		if ( $guidestatus != $autog ) then
			set lastmiss=`$xmlrpc --quiet -G TELE.guide_miss`
			set nowdate=`date +%s`
			if ( $guideon &lt; $nowdate ) then
				rts2-logcom "System should guide, but guider_status is $guidestatus (missed $lastmiss)."
			endif
			if ( $nextautog &lt; $nowdate ) then
				<xsl:copy-of select="$grabguide"/>
				rts2-logcom "Autoguider image saved in $lastgrab; trying to guide again"
				<xsl:copy-of select='$guidetest'/>
				if ( $new_guide == "ON" ) then
				   if ( $gdiff != 0 ) then
					tele gfoc $gdiff
				   endif
				endif
			endif
		endif
	endif

	set exposure_autoadjust=`$RTS2/bin/rts2-json -G XMLRPC.exposure_adjust`
	if ( ${?exposure} &amp;&amp; $exposure_autoadjust == 1 ) then
		set starfl=`$RTS2/bin/rts2-json --get-int IMGP.flux_A`
		set starmin=`$RTS2/bin/rts2-json --get-int XMLRPC.starfl_min`
		set starmax=`$RTS2/bin/rts2-json --get-int XMLRPC.starfl_max`
		if ( $starfl &lt; $starmin ) then
			rts2-logcom 'Expanding exposure time by 5 seconds'
			set exposure=`echo $exposure | awk '{ printf "%i", $1 + 5 }'`
		endif
		if ( $starfl &gt; $starmax ) then
			rts2-logcom 'Shortening exposure time by 5 seconds'
			set exposure=`echo $exposure | awk '{ printf "%i", $1 - 5 }'`
		endif
	else
		set exposure=<xsl:value-of select='@length'/>
	endif

	rts2-logcom "Starting $actual_filter exposure $imgid ($exposure sec) at `date`"
	<xsl:copy-of select='$abort'/>
#	if ( $tar_id != 3 &amp;&amp; $defoc_current == 0 ) then
	if ( $tar_id != 3 ) then
	      source $rts2dir/rts2.tempfocus
#       SWITCH to deltafocus for obs other than findfwhm obs EF 3/13/13
#	      source $rts2dir/rts2.deltafocus
#       Back to tempfocus EF 3/13/14
	endif
	ccd gowait $exposure
	<xsl:copy-of select='$abort'/>
	dstore
	set json_ret=0
	set fwhm2=`$RTS2/bin/rts2-json -G IMGP.fwhm_KCAM_2` &amp;&amp; set flux=`$RTS2/bin/rts2-json -G IMGP.flux_A` &amp;&amp; set peak=`$RTS2/bin/rts2-json -G IMGP.peak_A`
	if ( $? != 0 ) then
		rts2-logcom "Exposure done; last image data are not available, please check if IMGP module is running (if just restarted, wait for next image)"
	else
		if ( $last_obs_id == $obs_id ) then
			rts2-logcom "Exposure done; offsets " `printf '%+0.2f" %+0.2f" FWHM %.2f" FL %.0f MFL %.0f' $ora_l $odec_l $fwhm2 $flux $peak`
		else
			rts2-logcom "Exposure done; last flux " `printf '%.0f' $flux`
		endif
	endif
	$xmlrpc -c SEL.next
	if ( ${?imgdir} == 0 ) set imgdir=/rdata`grep "cd" /tmp/iraf_logger.cl |cut -f2 -d" "`
	set lastimage=`ls ${imgdir}[0-9]*.fits | tail -n 1`
	$xmlrpc -c "IMGP.only_process $lastimage $obs_id $imgid"
	set avrg=`$xmlrpc --quiet -G IMGP.average`
	if ( $? == 0 &amp;&amp; `echo $avrg '&lt;' 200 | bc` == 1 ) then
		rts2-logcom "Average value of image $lastimage is too low - $avrg, expected at least 200"
		echo "This is probably a problem with the KeplerCam controller. Restart the CCD driver, and then call again GOrobot. Current observation will be aborted."
		set continue=0
		exit
	endif
	if ( $xpa == 1 ) then
		xpaset ds9 fits mosaicimage iraf &lt; $lastimage
		xpaset -p ds9 zoom to fit
		xpaset -p ds9 scale scope global
	endif
	@ imgid ++
	<xsl:copy-of select='$printd'/> 'exposure sequence done' 
endif
<xsl:copy-of select='$abort'/>
</xsl:template>

<xsl:template match='exe'>
if ( $continue == 1 ) then
<!--	if ( '<xsl:value-of select='@path'/>' == '-' ) then
		set go = 0
		while ( $go == 0 )
			echo -n 'Command (or go to continue with the robot): '
			set x = "$&lt;"
			if ( "$x" == 'go' ) then
				set go = 1
			else
				eval $x
			endif
		end
	else  -->
		<xsl:copy-of select='$printd'/> rts2-logcom 'executing script <xsl:value-of select='@path'/>'
		source <xsl:value-of select='@path'/>
<!--	fi -->
endif
</xsl:template>

<xsl:template match="set">
<xsl:if test='@value = "filter"'>
if ( $continue == 1 ) then
	<xsl:copy-of select='$printd'/> "before filter"
	source $RTS2/bin/rts2_tele_filter <xsl:value-of select='@operands'/>
	if ( $? == 1 ) then
		unset exposure
	endif	
	set actual_filter="<xsl:value-of select='@operands'/>"
	<xsl:copy-of select='$printd'/> "after filter"
endif
</xsl:if>
<xsl:if test='@value = "ampcen"'>
if ( $continue == 1 ) then
	set ampstatus=`$xmlrpc --quiet -G TELE.ampcen`
	if ( $ampstatus != <xsl:value-of select='@operands'/> ) then
		tele ampcen <xsl:value-of select='@operands'/>
		rts2-logcom 'Ampcen set to <xsl:value-of select='@operands'/>'
	<xsl:if test='$debug != 0'>
	else
		echo `date +"%D %T.%3N %Z"` "ampcen already on $ampstatus, not changing it"
	</xsl:if>	
	endif
	set lastamp=<xsl:value-of select='@operands'/>
endif
</xsl:if>
<xsl:if test='@value = "autoguide"'>
if ( $continue == 1 ) then
	set new_guide=<xsl:value-of select='@operands'/>
	if ( $new_guide == 1 || $new_guide == "ON" || $new_guide == "on" ) then
		set new_guide = "ON"
	else
		set new_guide = "OFF"
	endif
	if ( $autog != $new_guide ) then
		$xmlrpc --quiet -c TELE.info
		set guidestatus=`$xmlrpc --quiet -G TELE.autog`
		if ( $guidestatus == 1 ) then
			set guidestatus = "ON"
		else
			set guidestatus = "OFF"
		endif
		if ( $guidestatus != $new_guide ) then
			if ( $new_guide == "ON" ) then
				rts2-logcom "Commanding autoguiding to ON"
				set nowdate=`date +%s`
				<xsl:copy-of select='$guidetest'/>
			else
				rts2-logcom "Set guiding to OFF"
				tele autog OFF
				#tele ggain Off
			endif
		<xsl:if test='$debug != 0'>	
		else
			echo `date +"%D %T.%3N %Z"` "autog already in $guidestatus status, not changing it"
		</xsl:if>	
		endif
		set autog=$new_guide
	<xsl:if test='$debug != 0'>	
	else
		echo `date +"%D %T.%3N %Z"` 'autog already set, ignoring autog request'
	</xsl:if>	
	endif	  	
endif
</xsl:if>
<xsl:if test='@value = "FOC_TOFF" and @device = "FOC"'>
set defoc_toffs=`echo $defoc_toffs + <xsl:value-of select='@operands'/> | bc`
</xsl:if>
</xsl:template>

<xsl:template match="for">
<xsl:variable name='count' select='generate-id(.)'/>
@ count_<xsl:value-of select='$count'/> = 0
<xsl:copy-of select='$abort'/>
while ($count_<xsl:value-of select='$count'/> &lt; <xsl:value-of select='@count'/> &amp;&amp; $continue == 1) <xsl:for-each select='*'><xsl:apply-templates select='current()'/></xsl:for-each>
@ count_<xsl:value-of select='$count'/> ++

end
</xsl:template>

<xsl:template match="acquire">
<!-- handles astrometry corrections -->
if ( ! (${?last_acq_obs_id}) ) @ last_acq_obs_id = 0

if ( $last_acq_obs_id != $obs_id ) then
	rts2-logcom "Starting acquisition/centering for observation with ID $obs_id"
	source $RTS2/bin/rts2_tele_filter i
	object test
<!--	tele ampcen 0 -->
	<xsl:copy-of select='$printd'/> "ampcen to 0"
	if ( $autog == 'ON' ) then
		tele autog OFF
		$autog = 'OFF'
	endif
	set pre_l = `echo "<xsl:value-of select='@precision'/> * 3600.0" | bc`
	@ pre = `printf '%.0f' $pre_l`
	@ err = $pre + 1
	@ maxattemps = 5
	@ attemps = $maxattemps
	while ( $continue == 1 &amp;&amp; $err &gt; $pre &amp;&amp; $attemps &gt; 0 )
		@ attemps --
		<xsl:copy-of select='$abort'/>
		rts2-logcom 'Starting i acquisition <xsl:value-of select='@length'/> sec exposure'
		ccd gowait <xsl:value-of select='@length'/>
		<xsl:copy-of select='$abort'/>
		dstore
		rts2-logcom 'Acquisition exposure done'
		<xsl:copy-of select='$abort'/>
		if ( $continue == 1 ) then
			if ( ${?imgdir} == 0 ) set imgdir=/rdata`grep "cd" /tmp/iraf_logger.cl |cut -f2 -d" "`
			set lastimage=`ls ${imgdir}[0-9]*.fits | tail -n 1`
			<!-- run astrometry, process its output -->	
			rts2-logcom "Running astrometry on $lastimage, $obs_id $imgid"
			foreach line ( "`/home/petr/rts2-sys/bin/img_process $lastimage $obs_id $imgid`" )
				echo "$line" | grep "^corrwerr" &gt; /dev/null
				if ( $? == 0 ) then
					set l=`echo $line`
					set ora_l = `echo "$l[5] * 3600.0" | bc`
					set odec_l = `echo "$l[6] * 3600.0" | bc`
					set ora = `printf '%.0f' $ora_l`
					set odec = `printf '%.0f' $odec_l`
					set err = `echo "$l[7] * 3600.0" | bc`
					set err = `printf '%.0f' $err`
					if ( $err > $pre ) then
						rts2-logcom "Acquiring: offset by $ora $odec ( $ora_l $odec_l ), error is $err arcsec"
						tele offset $ora $odec
                                                sleep 3
                                                rts2-logcom "FIX POINTING: set telescope position after offsets."
                                                tele set
						@ err = 0
					else
						rts2-logcom "Error is less than $pre arcsec ( $ora_l $odec_l ), stop acquisition"
						@ err = 0
					endif
					@ last_acq_obs_id = $obs_id
				endif
			end
			@ imgid ++
		endif
	end
	if ( $attemps &lt;= 0 ) then
		rts2-logcom "maximal number of attempts exceeded"
		echo "Pointing cannot be verified with astrometry. Something wrong with the telescope (e.g. on a limit), or it  is very cloudy. Check the telescope, clear the error and restart Rob" &gt; /pool/weather/robot.error
		set continue=0
		exit
	endif
	object $name
 <!-- 	tele ampcen $lastamp  -->
	<xsl:copy-of select='$printd'/> "ampcen to $lastamp"
<xsl:if test='$debug != 0'>
else
	rts2-logcom "already acquired for $obs_id"
</xsl:if>	
endif	
</xsl:template>

</xsl:stylesheet>
