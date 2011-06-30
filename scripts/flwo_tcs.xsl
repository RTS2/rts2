<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' indent='no'/>

<xsl:template match='/'>
@ imgid = 1
set continue=1
unset imgdir
set xpa=0
xpaget ds9 >&amp; /dev/null
if ( $? == 0 ) set xpa=1

set xmlrpc="$RTS2/bin/rts2-xmlrpcclient --config $XMLRPCCON"

set lastra=0
set lastdec=0

<xsl:apply-templates select='*'/>
</xsl:template>

<xsl:variable name='abort'>
if ( -e $rts2abort ) then
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
		if ( ${%ora} > 0 &amp;&amp; ${%odec} > 0 &amp;&amp; $ora &lt; 500 &amp;&amp; $odec &lt; 500 ) then
		  	set rra=`expr $ora - $lastra`
			set rdec=`expr $odec - $lastdec`
			if ( $rra &gt; -5 &amp;&amp; $rra &lt; 5 ) then
				set rra = 0
			endif
			if ( $rdec &gt; -5 &amp;&amp; $rdec &lt; 5 ) then
				set rdec = 0
			endif
			echo `date` "offseting $rra $rdec ($ora $odec; $lastra $lastdec)"
			if ( $rra != 0 || $rdec != 0 ) then
				tele offset $rra $rdec
				set lastra=$ora
				set lastdec=$odec
			endif	
		else
			echo `date` too big offset $ora $odec
		endif	  	
	else
		echo `date` "not offseting - correction from different target (observing $name, correction from $cname)"  
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
		echo `date` 'executing script <xsl:value-of select='@path'/>'
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
		echo -n `date` 'set ampcen to <xsl:value-of select='@operands'/>'
		tele ampcen <xsl:value-of select='@operands'/>
		echo '.'
	else
		echo `date` "ampcen already on $ampstatus, not changing it"
	endif
endif
</xsl:if>
<xsl:if test='@value = "autoguide"'>
if ( $continue == 1 ) then
	set guidestatus=`tele autog ?`
	if ( $guidestatus != <xsl:value-of select='@operands'/> ) then
		echo -n `date` 'set autog to <xsl:value-of select='@operands'/>'
		tele autog <xsl:value-of select='@operands'/>
		echo '.'
	else
		echo `date` "autog already in $guidestatus status, not changing it"
	endif
endif
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
