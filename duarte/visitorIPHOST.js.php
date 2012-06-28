
/*
Author: Robert Hashemian
http://www.hashemian.com/

You can use this code in any manner so long as the author's
name, Web address and this disclaimer is kept intact.
********************************************************
Usage Sample:

<script language="JavaScript">
VIH_BackColor = "palegreen";
VIH_ForeColor = "navy";
VIH_FontPix = "16";
VIH_DisplayFormat = "You are visiting from:<br>IP Address: %%IP%%<br>Host: %%HOST%%";
VIH_DisplayOnPage = "yes";
</script>
<script language="JavaScript" src="http://www.hashemian.com/js/visitorIP.js.php"></script>
*/

if (typeof(VIH_BackColor)=="undefined")
  VIH_BackColor = "white";
if (typeof(VIH_ForeColor)=="undefined")
  VIH_ForeColor= "black";
if (typeof(VIH_FontPix)=="undefined")
  VIH_FontPix = "16";
if (typeof(VIH_DisplayFormat)=="undefined")
  VIH_DisplayFormat = "You are visiting from:<br>IP Address: %%IP%%<br>Host: %%HOST%%";
if (typeof(VIH_DisplayOnPage)=="undefined" || VIH_DisplayOnPage.toString().toLowerCase()!="no")
  VIH_DisplayOnPage = "yes";

VIH_HostIP = "85.240.106.141";
VIH_HostName = "bl7-106-141.dsl.telepac.pt";

if (VIH_DisplayOnPage=="yes") {
  VIH_DisplayFormat = VIH_DisplayFormat.replace(/%%IP%%/g, VIH_HostIP);
  VIH_DisplayFormat = VIH_DisplayFormat.replace(/%%HOST%%/g, VIH_HostName);
  document.write("<table border='0' cellspacing='0' cellpadding='1' style='background-color:" + VIH_BackColor + "; color:" + VIH_ForeColor + "; font-size:" + VIH_FontPix + "px'><tr><td>" + VIH_DisplayFormat + "</td></tr></table>");
}
