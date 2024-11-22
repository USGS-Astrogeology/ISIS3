Unit test for Cube
Constructing cube ... 
File   = 
Samps  = 0
Lines  = 0
Bands  = 0
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 0
R/O    = N/A
R/W    = N/A
Lbytes = 65536

Creating 32-bit cube ... 
File   = IsisCube_00.cub
Samps  = 150
Lines  = 200
Bands  = 2
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 1
R/O    = 0
R/W    = 1
Lbytes = 65536

Write cube ... 
Opening cube ... 
File   = IsisCube_01.cub
Samps  = 150
Lines  = 200
Bands  = 2
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 1
R/O    = 1
R/W    = 0
Lbytes = 6563

Comparing cube ... 

Creating 8-bit cube ... 
Comparing cube ... 
File   = IsisCube_02.lbl
Samps  = 150
Lines  = 200
Bands  = 1
Base   = 200
Mult   = -1
Type   = 1
Atchd  = 0
Format = 0
Open   = 1
R/O    = 1
R/W    = 0
Lbytes = 419

Creating 16-bit cube ... 
Comparing cube ... 
File   = IsisCube_03.cub
Samps  = 150
Lines  = 200
Bands  = 2
Base   = 30000
Mult   = -1
Type   = 4
Atchd  = 1
Format = 1
Open   = 1
R/O    = 1
R/W    = 0
Lbytes = 65536

Testing histogram method, band 1 ... 
Average:        14900
Standard Dev:   8602.66
Mode:           149.148
Total Pixels:   30000
Null Pixels:    0

Testing histogram method, all bands ... 
Average:        29800
Standard Dev:   17205.2
Mode:           149.148
Total Pixels:   60000
Null Pixels:    0

**PROGRAMMER ERROR** Invalid band in [CubeInfo::Histogram].
**PROGRAMMER ERROR** Cannot create histogram object for an unopened cube.

Testing statistics method, band 1 ... 
Average:        14900
Standard Dev:   8602.66
Total Pixels:   30000
Null Pixels:    0

Testing statistics method, all bands ... 
Average:        29800
Standard Dev:   17205.2
Total Pixels:   60000
Null Pixels:    0

**PROGRAMMER ERROR** Invalid band in [CubeInfo::Statistics].
**PROGRAMMER ERROR** Cannot create statistics object for an unopened cube.

Virtual band tests
Nbands = 2
Band 1 = 1
Band 2 = 2

Nbands = 1
Band 1 = 2

ReOpen tests
File   = IsisCube_01.cub
Samps  = 150
Lines  = 200
Bands  = 1
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 1
R/O    = 1
R/W    = 0
Lbytes = 6563

File   = IsisCube_01.cub
Samps  = 150
Lines  = 200
Bands  = 1
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 1
R/O    = 0
R/W    = 1
Lbytes = 6563

File   = IsisCube_01.cub
Samps  = 150
Lines  = 200
Bands  = 1
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 1
R/O    = 1
R/W    = 0
Lbytes = 6563

Testing reading past cube boundaries ... 
Constructing cube ... 

File   = IsisCube_boundary.cub
Samps  = 10
Lines  = 10
Bands  = 4
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 1
Open   = 1
R/O    = 0
R/W    = 1
Lbytes = 65536

Reading completely within cube boundaries ... 
	Comparing results ... 
Reading completely outside band boundaries ... 
	Comparing results ... 

Reading partially within band boundaries ... 
	 Reading bands 0 (should be null) and 1 (should be 1.0)... 
		 Comparing results ... 
	 Reading bands 4 (should be 1.0) and 5 (should be null)... 
		 Comparing results ... 

Testing reading past cube boundaries with virtual bands (2, 1, 3, 4, 2)... 
Reading completely outside virtual band boundaries ... 
	Comparing results starting at band 6... 
	Comparing results starting at band 1000... 
	Comparing results starting at band -1... 

Reading partially within virtual band boundaries ... 
Comparing results ... 

Testing one line BSQ cube (where chunk dimensions == buffer shape) ... 
Constructing cube ... 

File   = IsisCube_bsqOneLine.cub
Samps  = 3
Lines  = 1
Bands  = 3
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 0
Open   = 1
R/O    = 0
R/W    = 1
Lbytes = 65536

Testing reading ascending repeating virtual bands (1, 2, 2, 3)... 

Testing reading skipped ascending virtual bands (1, 3, 3)... 

Testing reading outside of cube boundaries with virtual bands (1, 5)... 

Testing reading descending virtual bands (3, 1, 3)... 

Testing creating large BSQ where samples exceed 1GB chunk size limit ... 
Constructing cube ... 

File   = IsisCube_largebsq.cub
Samps  = 268435457
Lines  = 2
Bands  = 1
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 0
Open   = 1
R/O    = 0
R/W    = 1
Lbytes = 65536


Testing creating BSQ cube where size of sample pixels exceeds cube's lineCount ... 
Constructing cube ... 

File   = IsisCube_bsq.cub
Samps  = 15000
Lines  = 18000
Bands  = 1
Base   = 0
Mult   = 1
Type   = 7
Atchd  = 1
Format = 0
Open   = 1
R/O    = 0
R/W    = 1
Lbytes = 65536


Testing errors ... 
**PROGRAMMER ERROR** You already have a cube opened.
**PROGRAMMER ERROR** You already have a cube opened.
**PROGRAMMER ERROR** Cannot write to the cube [IsisCube_01.cub] because it is opened read-only.
**I/O ERROR** Unable to open [blah].
**PROGRAMMER ERROR** Out of array bounds [2].
**PROGRAMMER ERROR** Out of array bounds [0].
**PROGRAMMER ERROR** Try opening a file before you read it.
**PROGRAMMER ERROR** Tried to write to a cube before opening/creating it.
**PROGRAMMER ERROR** Number of samples [0], lines [0], or bands [0] cannot be less than 1.
**I/O ERROR** Label space is full in [IsisCube_04.cub] unable to write labels.
**USER ERROR** The cube you are attempting to create [IsisCube_05] is [33527GB]. This is larger than the current allowed size of [12GB]. The cube dimensions were (S,L,B) [1000000, 1000000, 9] with [4] bytes per pixel. If you still wish to create this cube, the maximum value can be changed in your personal preference file located in [~/.Isis/IsisPreferences] within the group CubeCustomization, keyword MaximumSize. If you do not have an ISISPreference file, please refer to the documentation 'Environment and Preference Setup'. Error.
**PROGRAMMER ERROR** Unknown value for access [a]. Expected 'r'  or 'rw'.
**PROGRAMMER ERROR** SetDimensions:  Invalid number of sample, lines or bands.
**PROGRAMMER ERROR** SetDimensions:  Invalid number of sample, lines or bands.
**PROGRAMMER ERROR** SetDimensions:  Invalid number of sample, lines or bands.
**I/O ERROR** Failed to open [...] with read/write access.
**ERROR** Cannot create the cube [shouldntExist.cub] with a pixel type set to None.
**PROGRAMMER ERROR** Cannot add a group to the label of cube [IsisCube_06.cub] because it is opened read-only.

Test creating an ecub
Object = IsisCube
  Object = Core
    ^DnFile = $ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub
  End_Object

  Group = TestGroup
  End_Group
End_Object

Object = Label
  Bytes = 65536
End_Object
End
0 0 0 20 20 20 39 39 39 0 20 39 0 20 39 0 20 39 
**ERROR** The cube [isisTruth_external.ecub] does not support storing DN data because it is using an external file for DNs.

Test creating an ecub from an ecub
Object = IsisCube
  Object = Core
    ^DnFile = isisTruth_external.ecub
  End_Object
End_Object

Object = Label
  Bytes = 65536
End_Object
End
0 0 0 20 20 20 39 39 39 0 20 39 0 20 39 0 20 39 
**ERROR** The cube [isisTruth_external2.ecub] does not support storing DN data because it is using an external file for DNs.

Test reading an ecub
Object = IsisCube
  Object = Core
    ^DnFile = $ISISTESTDATA/isis/src/base/unitTestData/isisTruth.cub
  End_Object

  Group = TestGroup
  End_Group

  Group = TestGroup2
  End_Group
End_Object

Object = Label
  Bytes = 65536
End_Object
End
0 0 0 20 20 20 39 39 39 0 20 39 0 20 39 0 20 39 
**PROGRAMMER ERROR** Cannot write to the cube [isisTruth_external.ecub] because it is opened read-only.

Test reading an ecub that points to another ecub
Object = IsisCube
  Object = Core
    ^DnFile = isisTruth_external.ecub
  End_Object
End_Object

Object = Label
  Bytes = 65536
End_Object
End
0 0 0 20 20 20 39 39 39 0 20 39 0 20 39 0 20 39 
**PROGRAMMER ERROR** Cannot write to the cube [isisTruth_external2.ecub] because it is opened read-only.
