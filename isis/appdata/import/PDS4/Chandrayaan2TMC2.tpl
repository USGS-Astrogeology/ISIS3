{% set file_name = Product_Observational.File_Area_Observational.File.file_name %}
{% set sensor = FindCH2Sensor(file_name) %}

{% set ImageArray = Product_Observational.File_Area_Observational.Array_2D_Image %}

Object = IsisCube
  Object = Core
    Group = Dimensions
      Samples = {{ ImageArray.Axis_Array.0.elements }}
      Lines   = {{ ImageArray.Axis_Array.1.elements }}
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
    SpacecraftName            = {{ Product_Observational.Observation_Area.Investigation_Area.name }}
    {% set inst_name = Product_Observational.Observation_Area.Observing_System.Observing_System_Component.1.name %}
    {% if inst_name == "terrain mapping camera" %}
    InstrumentId              = TMC-2
    {% endif %}
    TargetName                = {{ Product_Observational.Observation_Area.Target_Identification.name }}
    StartTime                 = {{ RemoveStartTimeZ(Product_Observational.Observation_Area.Time_Coordinates.start_date_time) }}
    StopTime                  = {{ RemoveStartTimeZ(Product_Observational.Observation_Area.Time_Coordinates.stop_date_time) }}
  End_Group

  Group = BandBin
    Center = 675
    Width = 175
  End_Group

  Group = Kernels
    NaifFrameCode = {% if sensor == "a" %}-152212
                    {% else if sensor == "f" %}-152211
                    {% else if sensor == "n" %}-152210
                    {% endif %}

  End_Group
End_Object

Object = Translation
End_Object
End
