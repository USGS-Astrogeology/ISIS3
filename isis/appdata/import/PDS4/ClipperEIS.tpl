{% set sensor = Product_Observational.Observation_Area.Observing_System.Observing_System_Component.1.name %}
{% set sub_sensor = Product_Observational.Observation_Area.Mission_Area.clipper_Observation_Information.clipper_EIS_Parameters.clipper_mode %}


{% set ImageArray = Product_Observational.File_Area_Observational.Array_2D_Image %}

Object = IsisCube
  Object = Core
    Group = Dimensions
      Samples = {{ ImageArray.Axis_Array.1.elements }}
      Lines   = {{ ImageArray.Axis_Array.0.elements }}
      Bands   = 1
    End_Group



    Group = Pixels
      {% set pixelType = ImageArray.Element_Array.data_type %}
      {% if exists("Product_Observational.File_Area_Observational.Array_2D_Image.Element_Array.data_type") %}
      Type       = {% if pixelType == "IEEE754LSBDouble" %} Double
                   {% else if pixelType == "IEEE754LSBSingle" %} Real
                   {% else if pixelType == "IEEE754MSBDouble" %} Double
                   {% else if pixelType == "IEEE754MSBSingle" %} Real
                   {% else if pixelType == "SignedByte" %} SignedByte
                   {% else if pixelType == "SignedLSB2" %} SignedWord
                   {% else if pixelType == "SignedLSB4" %} SignedInteger
                   {% else if pixelType == "SignedMSB2" %} SignedWord
                   {% else if pixelType == "SignedMSB4" %} SignedInteger
                   {% else if pixelType == "UnsignedByte" %} UnsignedByte
                   {% else if pixelType == "UnsignedLSB2" %} UnsignedWord
                   {% else if pixelType == "UnsignedLSB4" %} UnsignedInteger
                   {% else if pixelType == "UnsignedMSB2" %} UnsignedWord
                   {% else if pixelType == "UnsignedMSB4" %} UnsignedInteger
                   {% else %} Real
                   {% endif %}
      ByteOrder  = {% if pixelType == "IEEE754LSBDouble" %} LSB
                   {% else if pixelType == "IEEE754LSBSingle" %} LSB
                   {% else if pixelType == "IEEE754MSBDouble" %} MSB
                   {% else if pixelType == "IEEE754MSBSingle" %} MSB
                   {% else if pixelType == "SignedByte" %} LSB
                   {% else if pixelType == "SignedLSB2" %} LSB
                   {% else if pixelType == "SignedLSB4" %} LSB
                   {% else if pixelType == "SignedMSB2" %} MSB
                   {% else if pixelType == "SignedMSB4" %} MSB
                   {% else if pixelType == "UnsignedByte" %} LSB
                   {% else if pixelType == "UnsignedLSB2" %} LSB
                   {% else if pixelType == "UnsignedLSB4" %} LSB
                   {% else if pixelType == "UnsignedMSB2" %} MSB
                   {% else if pixelType == "UnsignedMSB4" %} MSB
                   {% else %} Lsb
                   {% endif %}
      {% else %}
      Type       = Real
      ByteOrder  = Lsb
      {% endif %}

      Base       = {% if exists("Product_Observational.File_Area_Observational.Array_2D_Image.Element_Array.value_offset") %}
                   {{ ImageArray.Element_Array.value_offset }}
                   {% else if exists("Product_Observational.File_Area_Observational.Array_2D_Image.offset._text") %}
                   {{ ImageArray.offset._text }}
                   {% else %}
                   0
                   {% endif %}
      Multiplier = {% if exists("Product_Observational.File_Area_Observational.Array_2D_Image.Element_Array.scaling_factor") %}
                   {{ ImageArray.Element_Array.scaling_factor._text }}
                   {% else %}
                   1
                   {% endif %}
    End_Group
  End_Object

  Group = Instrument
    SpacecraftName            = "{{ Product_Observational.Observation_Area.Observing_System.Observing_System_Component.0.name }}"
    InstrumentId              = "{{ sensor }}-{{ sub_sensor }}"
    TargetName                = {{ Product_Observational.Observation_Area.Target_Identification.name }}
    StartTime                 = {{ RemoveStartTimeZ(Product_Observational.Observation_Area.Time_Coordinates.start_date_time) }}
    ExposureDuration          = {{ Product_Observational.Observation_Area.Discipline_Area.img_Exposure.img_exposure_duration._text }} <{{ Product_Observational.Observation_Area.Discipline_Area.img_Exposure.img_exposure_duration.attrib_unit }}>
  End_Group

{% set filter=Product_Observational.Observation_Area.Mission_Area.clipper_Observation_Information.clipper_EIS_Parameters.clipper_filter_used %}
  Group = BandBin
    FilterName = {{ filter }}
    Center = {% if filter == "CLEAR" %}
               {% if sensor == "WAC" %}
               712.5 <nm>
               {% else if sensor == "NAC" %}
               702.5 <nm>
               {% else %}
               UNK
               {% endif %}
             {% else if filter == "NUV" %}
               {% if sensor == "WAC" %}
               387.5 <nm>
               {% else if sensor == "NAC"  %}
               377.5 <nm>
               {% else %}
               UNK
               {% endif %}
             {% else if filter == "BLU" %}
             427.5 <nm>
             {% else if filter == "GRN" %}
             555 <nm>
             {% else if filter == "RED" %}
             670 <nm>
             {% else if filter == "IR1" %}
             850 <nm>
             {% else if filter == "1MC" %}
             1000 <nm>
             {% else %}
             UNK
             {% endif %}
    Width = {% if filter == "CLEAR" %}
              {% if sensor == "WAC" %}
              675 <nm>
              {% else %}
              695 <nm>
              {% endif %}
            {% else if filter == "NUV" %}
              {% if sensor == "WAC" %}
              25 <nm>
              {% else %}
              45 <nm>
              {% endif %}
            {% else if filter == "BLU" %}
            95 <nm>
            {% else if filter == "GRN" %}
            70 <nm>
            {% else if filter == "RED" %}
            60 <nm>
            {% else if filter == "IR1" %}
            40 <nm>
            {% else if filter == "1MC" %}
            100 <nm>
            {% else %}
            UNK
            {% endif %}
  End_Group

  Group = Kernels
    NaifFrameCode = {% if sensor == "WAC" %}
                    -159104
                    {% else if sensor == "NAC" %}
                    -159103
                    {% else %}
                    UNK
                    {% endif %}

  End_Group
End_Object

Object = Translation
  {% if sub_sensor == "PUSHBROOM" %}
  Object = AncillaryProcess
    ProcessFunction = clipperEisPBCreateLineTable
  End_Object
  {% endif %}
End_Object
End
