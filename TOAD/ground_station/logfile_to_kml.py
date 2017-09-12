#!/usr/bin/env python3

import sys
import struct

# Useage
if len(sys.argv) != 2:
    print("Usage: {} <logfile.bin>".format(sys.argv[0]))
    sys.exit(1)

# Message Type Definitions
MESSAGE_PVT         = 1


## Print KML Header
print("<?xml version='1.0' encoding='UTF-8'?>")
print("<kml xmlns='http://www.opengis.net/kml/2.2'>")
print("<Document>")
print("    <name>Paths</name>")
print("    <description>TOAD LOG</description>")
print("   <Style id='yellowLineGreenPoly'>")
print("      <LineStyle>")
print("        <color>7f00ffff</color>")
print("        <width>4</width>")
print("      </LineStyle>")
print("      <PolyStyle>")
print("        <color>7f00ff00</color>")
print("      </PolyStyle>")
print("    </Style>")
print("    <Placemark>")
print("      <name>Absolute Extruded</name>")
print("      <description>Transparent green wall with yellow outlines</description>")
print("      <styleUrl>yellowLineGreenPoly</styleUrl>")
print("      <LineString>")
print("        <extrude>1</extrude>")
print("        <tessellate>1</tessellate>")
print("        <altitudeMode>relativeToGround</altitudeMode>")
print("       <coordinates>")
        
# Open log file
with open(sys.argv[1], 'rb') as log:

    # Read File
    log.read();

    # File pointer
    i = 0
    num_bytes = log.tell()
    
    # Loop until EOF
    while i in range(num_bytes):
        
        # Seek to next log
        log.seek(i)
        
        # Read Metadata
        header = log.read(6)    
        
        # Get Message Metadata
        meta_data = struct.unpack('<BBI', header)
        log_type = meta_data[0]
        toad_id = meta_data[1]
        systick = meta_data[2]
        systick /= 10000.0
        
        # Handle PVT Message
        if (log_type == MESSAGE_PVT):
                       
            payload = log.read(92)
            pvt = struct.unpack('<IHBBBBBBIiBBBBiiiiIIiiiiiIIHHIiI', payload)            
            print("         ", (pvt[14]/10000000), ", ", (pvt[15]/10000000), ", ", (pvt[17]/1000))
                              
        # Increment file pointer
        i += 128
        
## Print KML Footer 
print("        </coordinates>")
print("      </LineString>")
print("    </Placemark>")
print("  </Document>")
print("</kml>")
