<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' indent='no'/>

<xsl:template match="exposure">
echo 'Starting exposure <xsl:value-of select='@length'/>'
ccd gowait <xsl:value-of select='@length'/>
dstore
echo 'exposure done'
</xsl:template>

<xsl:template match="set">
<xsl:if test='@value = "filter"'>
echo -n 'Moving filter wheel to <xsl:value-of select='@operands'/>'
source $RTS2/rts2_tele_filter <xsl:value-of select='@operands'/>
</xsl:if>
</xsl:template>

<xsl:template match="for">
@ count = 0
while ($count &lt;= <xsl:value-of select='@count'/>)<xsl:for-each select='*'><xsl:apply-templates select='current()'/></xsl:for-each>end
</xsl:template>

</xsl:stylesheet>
