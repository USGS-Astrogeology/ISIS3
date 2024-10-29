{#- This template is used to determine the path and file name of the archive type, mission and instrument -#}
{#- specific template used to import an image file to a cube file -#}
{#- All output from the rendering of this template is used as the file name for a template to create ISIS cube labels -#}
{#- Only the last statement in this template should render any text, including \n after comments, thus the dashes -#}
{#- -#}
{#- Three variabls are set ImportSubDir, SpacecraftId, InstrumentId -#}
{#- They are combined into a relative_path/file_name.tpl in the last line -#}
{#- in the form ImportSubDir/SpacecraftIdInstrumentId.tpl -#}
{#- -#}
{#- Decide which import directory to find the template -#}
{%- if exists("Product_Observational.product_class") -%}
  {%- set ImportSubDir="PDS4" -%}
{%- else if exists("Product_Observational.Identification_Area.product_class") -%}
  {%- set ImportSubDir="PDS4" -%}
{%- else if exists("CCSD3ZF0000100000001NJPL3IF0PDS200000001") -%}
  {%- set ImportSubDir="PDS3" -%}
{%- else if exists("PDS_VERSION_ID") -%}
  {%- set ImportSubDir="PDS3" -%}
{%- endif -%}

{#- Set the instrument ID portion of the template file name -#}
{%- if exists("Product_Observational.Observation_Area.Observing_System.Observing_System_Component.1.name") -%}
{%- set InstrumentId=Product_Observational.Observation_Area.Observing_System.Observing_System_Component.1.name -%}
{%- else if exists("INSTRUMENT_ID.Value") -%}
{%- set InstrumentId=INSTRUMENT_ID.Value -%}
{%- endif -%}

{#- Get the spacecraft/mission name from the data -#}
{%- if exists("Product_Observational.Observation_Area.Investigation_Area.name") -%}
  {%- set SpacecraftName=Product_Observational.Observation_Area.Investigation_Area.name -%}
{%- else if exists("Product_Observational.Observation_Area.Investigation_Area.Instrument_Host_Name") -%}
  {%- set SpacecraftName=Product_Observational.Observation_Area.Investigation_Area.Instrument_Host_Name -%}
{%- else if exists("SPACECRAFT_NAME") -%}
  {%- set SpacecraftName=SPACECRAFT_NAME.Value -%}
{%- else if exists("QUBE.MISSION_NAME") -%}
  {%- set SpacecraftName=QUBE.MISSION_NAME.Value -%}
{%- else if exists("QUBE.ISIS_INSTRUMENT.SPACECRAFT_NAME") -%}
  {%- set SpacecraftName=QUBE.ISIS_INSTRUMENT.SPACECRAFT_NAME.Value -%}
{%- else if exists("SPACECRAFT_NAME") -%}
  {%- set SpacecraftName=SPACECRAFT_NAME.Value -%}
{%- else if exists("MISSION_NAME") -%}
  {%- set SpacecraftName=MISSION_NAME.Value -%}
{%- else if exists("INSTRUMENT_HOST_NAME") -%}
  {%- set SpacecraftName=INSTRUMENT_HOST_NAME.Value -%}
{%- endif -%}

{#- Use the SpacecraftName to set the SpacecraftId portion of the template file name -#}
{#- The InstrumentId may be adjusted once the spacecraft/mission name is found -#}
{#- This is useful when a single template can be used for multiple instruments -#}
{#- or when the instrument name is long or contains spaces, ... -#}
{%- if SpacecraftName == "TRACE GAS ORBITER" -%}
  {%- set SpacecraftId="TGO" -%}
{%- else if SpacecraftName == "Europa Clipper" -%}
  {%- set SpacecraftId="Clipper" -%}
  {%- set InstrumentId="EIS" -%}
{%- else if SpacecraftName == "VIKING_ORBITER_1" or SpacecraftName == "VIKING_ORBITER_2" -%}
  {%- set SpacecraftId="Viking" -%}
  {%- set InstrumentId="VIS" -%}
{%- else if SpacecraftName == "MARS_RECONNAISSANCE_ORBITER" or SpacecraftName == "MARS_RECON_ORBITER" -%}
  {%- set SpacecraftId="MRO" -%}
{%- else if SpacecraftName == "CASSINI-HUYGENS" -%}
  {%- set SpacecraftId="Cassini" -%}
    {%- if exists("QUBE") -%}
      {%- set InstrumentId="VIMS" -%}
    {%- else -%}
      {%- set InstrumentId="ISS" -%}
    {%- endif -%}
{%- else if SpacecraftName == "MARS_GLOBAL_SURVEYOR" -%}
  {%- set SpacecraftId="MGS" -%}
  {%- set InstrumentId="MOC" -%}
{%- else if SpacecraftName == "DAWN MISSION TO VESTA AND CERES" or SpacecraftName == "DAWN"-%}
  {%- set SpacecraftId="Dawn" -%}
  {%- if InstrumentId == "FC1" or InstrumentId == "FC2" -%}
    {%- set InstrumentId="FC"-%}
  {%- endif -%}
{%- else if SpacecraftName == "LUNAR RECONNAISSANCE ORBITER" -%}
  {%- set SpacecraftId="LRO" -%}
  {%- set InstrumentId="CNac" -%}
{%- else if SpacecraftName == "MARS EXPLORATION ROVER" -%}
  {%- set SpacecraftId="Mer" -%}
{%- else if SpacecraftName == "LUNAR_ORBITER_3" or  SpacecraftName == "LUNAR_ORBITER_4" or  SpacecraftName == "LUNAR_ORBITER_5" -%}
  {%- set SpacecraftId="Lo" -%}
  {%- set InstrumentId="HRC" -%}
{%- else if SpacecraftName == "SELENE" or SpacecraftName == "SELENE-M"-%}
  {%- set SpacecraftId="Kaguya" -%}
  {%- if InstrumentId == "MI-NIR" or InstrumentId == "MI-VIS"-%}
    {%- set InstrumentId="MI" -%}
  {%- else if InstrumentId == "TC" -%}
    {%- set InstrumentId="TC" -%}
  {%- endif -%}
{%- else if SpacecraftName == "ROSETTA-ORBITER"-%}
  {%- set SpacecraftId="Ros" -%}
  {%- if InstrumentId == "OSINAC" or InstrumentId == "OSIWAC"-%}
    {%- set InstrumentId="Osiris" -%}
  {%- endif -%}
{%- else if SpacecraftName == "Chandrayaan-2"-%}
  {%- set SpacecraftId="Chandrayaan2" -%}
  {%- if InstrumentId == "terrain mapping camera"-%}
    {%- set InstrumentId="TMC2" -%}
  {%- endif -%}
{%- endif -%}

{#- Combine the pieces to output the file name to be used to import an image into a cube -#}
{#- This is the only line in this file that should output anything -#}
{%- if SpacecraftId -%}$ISISROOT/appdata/import/{{- ImportSubDir -}}/{{- SpacecraftId -}}{{- InstrumentId -}}.tpl{%- endif -%}

