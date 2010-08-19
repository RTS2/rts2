<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' indent='no'/>

<xsl:variable name='abort'>
if ( -e $rts2abort ) exit
</xsl:variable>

<xsl:template match="exposure">
echo 'Starting exposure <xsl:value-of select='@length'/>'
<xsl:copy-of select='$abort'/>
ccd gowait <xsl:value-of select='@length'/>
<xsl:copy-of select='$abort'/>
dstore
echo 'exposure done'
<xsl:copy-of select='$abort'/>
</xsl:template>

<xsl:template match="set">
<xsl:if test='@value = "filter"'>
echo -n 'Moving filter wheel to <xsl:value-of select='@operands'/>'
source $RTS2/bin/rts2_tele_filter <xsl:value-of select='@operands'/>
</xsl:if>
</xsl:template>

<xsl:template match="for">
@ count = 0
<xsl:copy-of select='$abort'/>
while ($count &lt;= <xsl:value-of select='@count'/>)<xsl:for-each select='*'><xsl:apply-templates select='current()'/></xsl:for-each>end
</xsl:template>

</xsl:stylesheet>
