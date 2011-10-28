<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' indent='no'/>

<xsl:template match='/'>

if ( ! (${?imgid}) ) @ imgid = 1

if ( ! (${?lastoffimage}) ) set lastoffimage=-1

if ( ! (${?last_offtarget}) ) set last_offtarget=-1
	
# next autoguider attempt
if ( ! (${?nextautog}) ) set nextautog=`date +%s`

set continue=1
unset imgdir
set xpa=0
# XPA takes way too long to queury. Uncomment this if you would like to check for it anyway, but expect ~0.2 sec penalty
# xpaget ds9 >&amp; /dev/null
# if ( $? == 0 ) set xpa=1

set defoc_toffs=0

if ( ! (${?autog}) ) set autog='UNKNOWN'

<!-- rts2-logcom "script running" -->

<xsl:apply-templates select='*'/>

</xsl:template>

<xsl:variable name='abort'>
if ( -e $rts2abort ) then
	rts2-logcom 'aborting observations. Please wait'
	source $RTS2/bin/.rts2-runabort
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
	rts2-logcom "interrupting target $name ($tar_id)"
endif
</xsl:variable>

<!-- called after guider is set to ON. Wait for some time for guider to settle down,
  and either find guide star, or give up-->
<xsl:variable name='guidetest'>
	tele autog ON
	@ retr = 5
	while ( $retr &gt;= 0 )
		set gs=`tele autog ?`
		if ( $gs == 'ON' ) then
			break
		endif
		$xmlrpc --quiet -c TELE.info
		set fwhms=`$xmlrpc --quiet -G TELE.fwhms`
		set guide_seg=`$xmlrpc --quiet -G TELE.guide_seg`
		set isguiding=`$xmlrpc --quiet -G TELE.isguiding`
		rts2-logcom "autoguider started, but star still not acquired. FWHMS $fwhms SEG $guide_seg isguiding $isguiding"
		@ retr --
	end
	if ( $retr &gt; 0 ) then
		rts2-logcom "successfully switched autoguider to ON"
	else
		@ nextautog = $nowdate + 300
		set textdate = `awk 'BEGIN { print strftime("%T"'",$nextautog); }"`
		rts2-logcom "autoguider command failed, will tray again at $textdate"
	endif	
</xsl:variable>

<xsl:template match="disable">
$RTS2/bin/rts2-target -d $tar_id
</xsl:template>

<xsl:template match="tempdisable">
$RTS2/bin/rts2-target -n +<xsl:value-of select='.'/> $tar_id
</xsl:template>

<xsl:template match="exposure">
if ( $continue == 1 ) then
  	<!-- rts2-logcom "starting pre-exposure checks" -->
        set cname=`$xmlrpc --quiet -G IMGP.object`
	set ora_l=`$xmlrpc --quiet -G IMGP.ora`
	set odec_l=`$xmlrpc --quiet -G IMGP.odec`
	set ora=`echo $ora_l | sed 's#^\([-+0-9]*\).*#\1#'`
	set odec=`echo $odec_l | sed 's#^\([-+0-9]*\).*#\1#'`
	if ( $cname == $name ) then
		if ( ${%ora} &gt; 0 &amp;&amp; ${%odec} &gt; 0 &amp;&amp; $ora &gt; -500 &amp;&amp; $ora &lt; 500 &amp;&amp; $odec &gt; -500 &amp;&amp; $odec &lt; 500 ) then
		  	set rra=$ora
			set rdec=$odec
			if ( $rra &gt; -5 &amp;&amp; $rra &lt; 5 ) then
				set rra = 0
			endif
			if ( $rdec &gt; -5 &amp;&amp; $rdec &lt; 5 ) then
				set rdec = 0
			endif

			set apply=`$xmlrpc --quiet -G IMGP.apply_corrections`
			set imgnum=`$xmlrpc --quiet -G IMGP.img_num`

			if ( $apply == 0 ) then
        			rts2-logcom "corrections disabled, do not apply correction $rra $rdec ($ora_l $odec_l) img_num $imgnum"	
			else

				set xoffs=`$xmlrpc --quiet -G IMGP.xoffs`
				set yoffs=`$xmlrpc --quiet -G IMGP.yoffs`

				$xmlrpc --quiet -c TELE.info
				set currg=`$xmlrpc --quiet -G TELE.autog`

				if ( $currg == '1' ) then
					rts2-logcom "autoguider is $currg - not offseting $rra $rdec ($ora_l $odec_l; $xoffs $yoffs) img_num $imgnum"
				else
					if ( $imgnum &lt;= $lastoffimage ) then
						rts2-logcom "older or same image received - not offseting $rra $rdec ($ora_l $odec_l; $xoffs $yoffs) img_num $imgnum lastimage $lastoffimage"
					else
						if ( $tar_id == $last_offtarget ) then
							$xmlrpc -s TELE.CORR_ -- "$ora_l $odec_l"
						else
							$xmlrpc -s TELE.OFFS -- "$ora_l $odec_l"
							set last_offtarget = $tar_id
						endif
						rts2-logcom "offseting $ora $odec from $rra $rdec ($ora_l $odec_l; $xoffs $yoffs) img_num $imgnum autog $autog"
						if ( $rra != 0 || $rdec != 0 ) then
							tele offset $rra $rdec
						endif
						set lastoffimage=$imgnum
						if ( ${?lastimage} ) then
							set lastoffimage=`echo $lastimage | sed 's#.*/\([0-9][0-9][0-9][0-9]\).*#\1#'`
						endif  
					endif
				endif
			endif
		else
			rts2-logcom "too big offset $ora_l $odec_l"
		endif	  	
<!---	else
		rts2-logcom "not offseting - correction from different target (observing $name, correction from $cname)"  -->
	endif
	set diff_l=`echo $defoc_toffs - $defoc_current | bc`
	if ( $diff_l != 0 ) then
		set diff=`printf '%+0f' $diff`
		rts2-logcom "offseting focus to $diff ( $defoc_toffs - $defoc_current )"
		set diff_f=`printf '%+02f' $diff`
		tele hfocus $diff_f
		set defoc_current=`echo $defoc_current + $diff_l | bc`
	else
		rts2-logcom "keep focusing offset ( $defoc_toffs - $defoc_current )"
	endif

	if ( $autog == 'ON' ) then
		$xmlrpc --quiet -c TELE.info
		set guidestatus=`$xmlrpc --quiet -G TELE.autog`
		if ( $guidestatus == 1 ) then
			set guidestatus = "ON"
		else
			set guidestatus = "OFF"
		endif
		if ( $guidestatus != $autog ) then
			rts2-logcom "system should guide, but autoguider is in $guidestatus. Grabing autoguider image"
			tele grab
			set lastgrab = `ls -rt /Realtime/guider/frames/0*.fits | tail -1`
			set dir=/Realtime/guider/frames/ROBOT_`date +%Y%m%d`
			mkdir -p $dir
			set autof=$dir/`date +%H%M%S`.fits
			cp $lastgrab $autof
			rts2-logcom "autoguider image saved in $autof"
			set nowdate=`date +%s`
			if ( $nextautog &lt; $nowdate ) then
				rts2-logcom "trying to guide again (switching autog to ON)"
				<xsl:copy-of select='$guidetest'/>
			endif
		endif
	endif

	echo `date` 'starting <xsl:value-of select='@length'/> sec exposure'
	<xsl:copy-of select='$abort'/>
	ccd gowait <xsl:value-of select='@length'/>
	<xsl:copy-of select='$abort'/>
	dstore
	$xmlrpc -c SEL.next
	if ( ${?imgdir} == 0 ) set imgdir=/rdata`grep "cd" /tmp/iraf_logger.cl |cut -f2 -d" "`
	set lastimage=`ls ${imgdir}[0-9]*.fits | tail -n 1`
	$RTS2/bin/rts2-image -i --camera KCAM --telescope FLWO48 --obsid $obs_id --imgid $imgid $lastimage
	set avrg=`$RTS2/bin/rts2-image -s $lastimage | cut -d' ' -f2`
	if ( `echo $avrg '&lt;' 200 | bc` == 1 ) then
		rts2-logcom "average value of image $lastimage is too low - $avrg, expected at least 200"
		rts2-logcom "this is probably problem with KeplerCam controller. Please proceed to restart"
		rts2-logcom "CCD driver, and then call again GOrobot"
		set continue=0
		exit
	endif
	$xmlrpc -c "IMGP.only_process $lastimage"
	if ( $xpa == 1 ) then
		xpaset ds9 fits mosaicimage iraf &lt; $lastimage
		xpaset -p ds9 zoom to fit
		xpaset -p ds9 scale scope global
	endif	
	@ imgid ++
	echo `date` 'exposure done'
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
		rts2-logcom 'executing script <xsl:value-of select='@path'/>'
		source <xsl:value-of select='@path'/>
<!--	fi -->
endif
</xsl:template>

<xsl:template match="set">
<xsl:if test='@value = "filter"'>
if ( $continue == 1 ) then
	source $RTS2/bin/rts2_tele_filter <xsl:value-of select='@operands'/>
endif
</xsl:if>
<xsl:if test='@value = "ampcen"'>
if ( $continue == 1 ) then
	set ampstatus=`$xmlrpc --quiet -G TELE.ampcen`
	if ( $ampstatus != <xsl:value-of select='@operands'/> ) then
		tele ampcen <xsl:value-of select='@operands'/>
		rts2-logcom 'set ampcen to <xsl:value-of select='@operands'/>'
	else
		echo `date` "ampcen already on $ampstatus, not changing it"
	endif
endif
</xsl:if>
<xsl:if test='@value = "autoguide"'>
if ( $continue == 1 ) then
	if ( $autog != <xsl:value-of select='@operands'/> ) then
		$xmlrpc --quiet -c TELE.info
		set guidestatus=`$xmlrpc --quiet -G TELE.autog`
		if ( $guidestatus == 1 ) then
			set guidestatus = "ON"
		else
			set guidestatus = "OFF"
		endif
		if ( $guidestatus != <xsl:value-of select='@operands'/> ) then
			if ( <xsl:value-of select='@operands'/> == "ON" ) then
				rts2-logcom "commanding autoguiding to ON"
				set nowdate=`date +%s`
				<xsl:copy-of select='$guidetest'/>
			else
				rts2-logcom "switching guiding to OFF"
				tele autog OFF
			endif
		else
			echo `date` "autog already in $guidestatus status, not changing it"
		endif
		set autog=<xsl:value-of select='@operands'/>
	else
		echo `date` 'autog already set, ignoring autog request'
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

if ( $last_acq_obs_id == $obs_id ) then
	echo `date` "already acquired for $obs_id"
else	
	rts2-logcom "acquiring for obsid $obs_id"
	tele filter i
	if ( $autog == 'ON' ) then
		tele autog OFF
		$autog = 'OFF'
	endif
	@ pre = `echo "<xsl:value-of select='@precision'/> * 3600.0" | bc | sed 's#^\([-+0-9]*\).*#\1#'`
	@ err = $pre + 1
	@ maxattemps = 5
	@ attemps = $maxattemps
	while ( $continue == 1 &amp;&amp; $err &gt; $pre &amp;&amp; $attemps &gt; 0 )
		@ attemps --
		echo `date` 'starting <xsl:value-of select='@length'/> sec exposure'
		<xsl:copy-of select='$abort'/>
		ccd gowait <xsl:value-of select='@length'/>
		<xsl:copy-of select='$abort'/>
		dstore
		<xsl:copy-of select='$abort'/>
		if ( $continue == 1 ) then
			if ( ${?imgdir} == 0 ) set imgdir=/rdata`grep "cd" /tmp/iraf_logger.cl |cut -f2 -d" "`
			set lastimage=`ls ${imgdir}[0-9]*.fits | tail -n 1`
			<!-- run astrometry, process its output -->	
			echo `date` "running astrometry on $lastimage"
			foreach line ( "`/home/petr/rts2-sys/bin/img_process $lastimage`" )
				echo "$line" | grep "^corrwerr" &gt; /dev/null
				if ( $? == 0 ) then
					set l=`echo $line`
					set ora_l = `echo "$l[5] * 3600.0" | bc`
					set odec_l = `echo "$l[6] * 3600.0" | bc`
					set ora = `echo $ora_l | sed 's#^\([-+0-9]*\).*#\1#'`
					set odec = `echo $odec_l | sed 's#^\([-+0-9]*\).*#\1#'`
					@ err = `echo "$l[7] * 3600.0" | bc | sed 's#^\([-+0-9]*\).*#\1#'`
					if ( $err > $pre ) then
						rts2-logcom "acquiring: offseting by $ora $odec ( $ora_l $odec_l ), error is $err arcsecs"
						tele offset $ora $odec
					else
						rts2-logcom "error is less than $pre arcsecs ( $ora_l $odec_l ), stop acquistion"
						@ err = 0
					endif
					@ last_acq_obs_id = $obs_id
				endif
			end
		endif
	end
	if ( $attemps &lt;= 0 ) then
		rts2-logcom "maximal number of attemps exceeded"
	endif  
endif	
</xsl:template>

</xsl:stylesheet>
