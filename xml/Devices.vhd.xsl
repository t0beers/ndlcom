<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" encoding="ascii" />
<xsl:strip-space elements="/" />

<xsl:template match="/">-- GENERATED FILE. DO NOT MODIFY, CHANGES WILL BE LOST!

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

package Devices is

    -- ---------- --
    -- Device Ids --
    -- ---------- --

<xsl:apply-templates select="//device"/>

end;

</xsl:template>

<!-- device elements -->
<xsl:template match="device">
<!-- combine each "device" node to be a constant: -->
<xsl:text>    constant Device_Id_</xsl:text><xsl:value-of select="@name"/>
<!-- the intermediate vhdl-line -->
<xsl:text> : std_logic_vector(7 downto 0) := </xsl:text>
<!-- and the final payload, the deviceid, directly as integer -->

<xsl:text>std_logic_vector(to_unsigned(</xsl:text><xsl:value-of select="@id"/><xsl:text>,8));</xsl:text>
<!-- only print a comment after the constant if somone actually wrote one -->
<xsl:if test="text() != ''"><xsl:text> -- </xsl:text><xsl:value-of select="text()"/></xsl:if>
<!-- and finally a newline if we are not the last item -->
<xsl:if test="(following-sibling::device)"><xsl:text>
</xsl:text></xsl:if>
</xsl:template>

</xsl:stylesheet>
