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
<xsl:apply-templates select='*'/>
</xsl:template>

<xsl:variable name='abort'>
if ( -e $rts2abort ) then
	source $RTS2/bin/.rts2-runabort
	rm -f $lasttarget
	exit
endif
set ms=`$RTS2/bin/rts2-xmlrpcclient --config $XMLRPCCON --master-state on`
if ( $? != 0 || $ms == 0 ) then
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
echo `date` 'starting <xsl:value-of select='@length'/> sec exposure'
<xsl:copy-of select='$abort'/>
ccd gowait <xsl:value-of select='@length'/>
<xsl:copy-of select='$abort'/>
dstore
if ( ${?imgdir} == 0 ) set imgdir=/rdata`grep "cd" /tmp/iraf_logger.cl |cut -f2 -d" "`
set lastimage=`ls ${imgdir}[0-9]*.fits | tail -n 1`
$RTS2/bin/rts2-image -i --camera KCAM --telescope FLWO48 --obsid $obs_id --imgid $imgid $lastimage
if ( $xpa == 1 ) then
	xpaset ds9 fits mosaicimage iraf &lt; $lastimage
	xpaset -p ds9 zoom to fit
	xpaset -p ds9 scale scope global
endif	
@ imgid ++
echo `date` 'exposure done'
<xsl:copy-of select='$abort'/>
</xsl:template>

<xsl:template match='exe'>
echo `date` 'executing script <xsl:value-of select='@path'/>'
source <xsl:value-of select='@path'/>
</xsl:template>

<xsl:template match="set">
<xsl:if test='@value = "filter"'>
echo -n `date` 'moving filter wheel to <xsl:value-of select='@operands'/>'
source $RTS2/bin/rts2_tele_filter <xsl:value-of select='@operands'/>
</xsl:if>
<xsl:if test='@value = "ampcen"'>
set ampstatus=`tele ampcen ?`
if ( $ampstatus != <xsl:value-of select='@operands'/> ) then
	tele ampcen <xsl:value-of select='@operands'/>
endif
</xsl:if>
<xsl:if test='@value = "autoguide"'>
set guidestatus=`tele autog ?`
if ( $guidestatus != <xsl:value-of select='@operands'/> ) then
	tele autog <xsl:value-of select='@operands'/>
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
