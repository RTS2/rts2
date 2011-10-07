<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' indent='no'/>

<xsl:template match='/'>
if ( ! (${?imgid}) ) then
	@ imgid = 1
endif

if ( ! (${?lastoffimage}) ) then
	set lastoffimage=-1
endif

# next autoguider attempt
set nextautog=`date +%s`

set continue=1
unset imgdir
set xpa=0
xpaget ds9 >&amp; /dev/null
if ( $? == 0 ) set xpa=1

set xmlrpc="$RTS2/bin/rts2-xmlrpcclient --config $XMLRPCCON"

set defoc_toffs=0

if ( ! (${?autog}) ) then
	set autog='UNKNOWN'
endif

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

<xsl:template match="disable">
$RTS2/bin/rts2-target -d $tar_id
</xsl:template>

<xsl:template match="tempdisable">
$RTS2/bin/rts2-target -n +<xsl:value-of select='.'/> $tar_id
</xsl:template>

<xsl:template match="exposure">
if ( $continue == 1 ) then
        set cname=`$xmlrpc --quiet -G IMGP.object`
	set ora=`$xmlrpc --quiet -G IMGP.ora | sed 's#^\([-+0-9]*\).*#\1#'`
	set odec=`$xmlrpc --quiet -G IMGP.odec | sed 's#^\([-+0-9]*\).*#\1#'`
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
        			rts2-logcom "corrections disabled, do not apply correction $rra $rdec ($ora $odec) img_num $imgnum"	
			else
				set xoffs=`$xmlrpc --quiet -G IMGP.xoffs`
				set yoffs=`$xmlrpc --quiet -G IMGP.yoffs`

				set currg=`tele autog ?`

				if ( $currg == 'ON' ) then
					rts2-logcom "autoguider is $currg - not offseting $rra $rdec ($ora $odec; $xoffs $yoffs) img_num $imgnum"
				else
					if ( $imgnum &lt;= $lastoffimage ) then
						rts2-logcom "older or same image received - not offseting $rra $rdec ($ora $odec; $xoffs $yoffs) img_num $imgnum lastimage $lastoffimage"
					else
						rts2-logcom "offseting $rra $rdec ($ora $odec; $xoffs $yoffs) img_num $imgnum autog $autog"
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
			rts2-logcom "too big offset $ora $odec"
		endif	  	
<!---	else
		rts2-logcom "not offseting - correction from different target (observing $name, correction from $cname)"  -->
	endif
	set diff=`echo $defoc_toffs - $defoc_current | bc`
	if ( $diff != 0 ) then
		rts2-logcom "offseting focus to $diff ( $defoc_toffs - $defoc_current )"
		set diff_f=`printf '%+02.f' $diff`
		tele hfocus $diff_f
		set defoc_current=`echo $defoc_current + $diff | bc`
	else
		rts2-logcom "keep focusing offset ( $defoc_toffs - $defoc_current )"
	endif

	if ( $autog == 'ON' ) then
		set guidestatus=`tele autog ?`
		if ( $guidestatus != $autog ) then
			rts2-logcom "system should guide, but autoguider is in $guidestatus. Grabing autoguider image"
			tele grab
			set lastgrab = `ls -rt /Realtime/guider/frames/0*.fits | tail -1`
			set dir=/Realtime/guider/frames/ROBOT_`date +"%Y%m%d"`
			mkdir $dir
			set autof=$dir/`date +"%H%M%S"`.fits
			cp $lastgrab $autof
			rts2-logcom "autoguider image saved in $autof"
			set nowdate=`date +"%s"`
			if ( $nextautog &lt; $nowdate ) then
				tele autog ON
				@ nextautog = $nowdate + 300
				rts2-logcom "tried again autoguider ON, next attempt at $nextautog"
			else
				rts2-logcom "autoguider timeout $nextautog not expired (now $nowdate)"
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
	echo -n `date` 'moving filter wheel to <xsl:value-of select='@operands'/>'
	source $RTS2/bin/rts2_tele_filter <xsl:value-of select='@operands'/>
endif
</xsl:if>
<xsl:if test='@value = "ampcen"'>
if ( $continue == 1 ) then
	set ampstatus=`tele ampcen ?`
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
		set guidestatus=`tele autog ?`
		if ( $guidestatus != <xsl:value-of select='@operands'/> ) then
			tele autog <xsl:value-of select='@operands'/>
			set nextautog=`date +"%s"`
			@ nextautog = $nextautog + 300
			rts2-logcom "set autog to <xsl:value-of select='@operands'/>, next autoguiding attempt not before $nextautog"
		else
			echo `date` "autog already in $guidestatus status, not changing it"
		endif
		set autog=<xsl:value-of select='@operands'/>
	else
		echo `date` 'autog already set, ignoring it'
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

</xsl:stylesheet>
