<?xml version="1.0" encoding="UTF-8"?>
<!--Created February 2006 for DITA message specialization-->
<!--This XSL splits one TMS XML file into separate message XML files and maps TMS XML tags to DITA tags-->
<xsl:stylesheet version="1.1" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:math="http://exslt.org/math"
	xmlns:saxon="http://icl.com/saxon"
	extension-element-prefixes="math saxon">



<!-- This is customization from LLM for the ID team to support XREF-->
<xsl:template match="@*|node()">
  <xsl:copy>
    <xsl:apply-templates select="@*|node()"/>
  </xsl:copy>
</xsl:template>
<xsl:template match="a">
  <xref><xsl:apply-templates select="node()|@href"/></xref>
</xsl:template>


</xsl:stylesheet>
