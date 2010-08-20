<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' indent='no'/>

<xsl:template match='/'>
@ imgid = 0
<xsl:apply-templates select='*'/>
</xsl:template>

<xsl:variable name='abort'>
if ( -e $rts2abort ) source $RTS2/bin/.rts2-runabort
</xsl:variable>

<xsl:template match="exposure">
echo `date` 'starting exposure <xsl:value-of select='@length'/>'
<xsl:copy-of select='$abort'/>
ccd gowait <xsl:value-of select='@length'/>
<xsl:copy-of select='$abort'/>
dstore
#$RTS2/bin/rts2image -i --camera KCAM --telescope FLWO48 --obsid $obs_id --imgid $imgid $image
echo `date` 'exposure done'
<xsl:copy-of select='$abort'/>
</xsl:template>

<xsl:template match="set">
<xsl:if test='@value = "filter"'>
echo -n `date` 'moving filter wheel to <xsl:value-of select='@operands'/>'
source $RTS2/bin/rts2_tele_filter <xsl:value-of select='@operands'/>
</xsl:if>
</xsl:template>

<xsl:template match="for">
@ count = 0
<xsl:copy-of select='$abort'/>
while ($count &lt;= <xsl:value-of select='@count'/>)<xsl:for-each select='*'><xsl:apply-templates select='current()'/></xsl:for-each>
@ count ++

end
</xsl:template>

</xsl:stylesheet>
