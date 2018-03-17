/**
 * Converts text exports from: Nobeltec Visual Navigation Suite
 *                         to: GPX xml file
 * 
 * Text exports supported:
 *      - Routes
 *      - Tracks
 * 
 * Text file must be either routes exclusively, or tracks exclusively.
 * *** UNEXPECTED BEHAVIOR if they are mixed in the same file.
 * 
 * Usage: togpx [text export file] [saved gpx file]
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


void write_garmin_headers(FILE* gpx)
{
    char* header = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
                   "<gpx xmlns=\"http://www.topografix.com/GPX/1/1\" xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\" xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\" creator=\"Oregon 400t\" version=\"1.1\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd http://www.garmin.com/xmlschemas/GpxExtensions/v3 http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd http://www.garmin.com/xmlschemas/TrackPointExtension/v1 http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd\">\n";
    fprintf(gpx, "%s", header);
}

void write_metadata(FILE* gpx)
{
    char* metadata = "<metadata>\n"
                     "    <link href=\"http://www.garmin.com\">\n"
                     "        <text>Garmin International</text>\n"
                     "    </link>\n"
                     "    <time>2009-10-17T22:58:43Z</time>\n"
                     "</metadata>\n";
    fprintf(gpx, "%s", metadata);
}

void write_footer(FILE* gpx)
{
    char* footer = "</gpx>";
    fprintf(gpx, "%s\n", footer);
}

int contains_line(FILE* file, char* str)
{
    char line[10000];
    while (fgets(line, 10000, file) != 0)
    {
        if (strcmp(line, str) == 0)
            return 1;
    }
    return 0;
}

int is_tracks(FILE* export)
{
    return contains_line(export, "Type = Track\r\n");
}

int is_routes(FILE* export)
{
    return contains_line(export, "Type = Route\r\n");
}

void readwrite_tracks(FILE* export, FILE* gpx)
{
    char line[10000];
    int start = 0;

    rewind(export);

    while (fgets(line, 10000, export) != 0)
    {
        if (start == 1)
        {
            int lat;
            float latminutes;
            int lng;
            float lngminutes;
            char date[11];
            char tim[10];
            char ns[2]; // north or south
            char ew[2]; // east or west

            if (strcmp("}}\r\n", line) == 0)
            {
                char* track_footer = "    </trkseg>\n"
                                     "</trk>\n";
                fprintf(gpx, "%s", track_footer);
                start = 0;
                continue;
            }

            // 50 39.92140 N 125 56.27690 W 2017-05-12 08:57:33Z
            if (sscanf(line, "%i %f %s %i %f %s %s %s", &lat, &latminutes, ns, &lng, &lngminutes, ew, date, tim) == 8)
            {
                float lattitude = (float)lat + (latminutes/60);
                float longitude = (float)lng + (lngminutes/60);
                char* track_points = "        <trkpt lat=\"%f\" lon=\"%f\">\n"
                                     "            <ele>0</ele>\n"
                                     "            <time>%sT%s</time>\n"
                                     "        </trkpt>\n";
                if (ns[0] == 'S')
                    lattitude *= -1.0;
                if (ew[0] == 'W')
                    longitude *= -1.0;
                fprintf(gpx, track_points, lattitude, longitude, date, tim);
                // printf("lat:  %f\n", (float)lat + (latminutes/60));
                // printf("lng:  %f\n", (float)lng + (lngminutes/60));
                // printf("date: %s\n", date);
                // printf("time: %s\n", tim);
            }
        }
        else if (strcmp("TrackMarks = {{\r\n", line) == 0)
        {
            char* track_header = "<trk>\n"
                                 "    <name>Example GPX Document</name>\n"
                                 "    <trkseg>\n";
            fprintf(gpx, "%s", track_header);
            start = 1;
        }
    }
}

void readwrite_routes(FILE* export, FILE* gpx)
{
    // char line[10000];
    // int route_started = 0;
    // int mark_started = 0;

    // int lat;
    // float latminutes;
    // int lng;
    // float lngminutes;
    // char date[11];
    // char tim[10];
    // char ns[2]; // north or south
    // char ew[2]; // east or west

    // rewind(export);

    // while (fgets(line, 10000, export) != 0)
    // {
    //     if (route_started == 1)
    //     {
    //         if (mark_started == 1)
    //         {
    //         }
    //         if (strcmp("Type = Mark\r\n", line) == 0)
    //         {
    //             char* track_footer = "    </trkseg>\n"
    //                                  "</trk>\n";
    //             fprintf(gpx, "%s", track_footer);
    //             route_started = 0;
    //             continue;
    //         }

    //         // 50 39.92140 N 125 56.27690 W 2017-05-12 08:57:33Z
    //         if (sscanf(line, "%i %f %s %i %f %s %s %s", &lat, &latminutes, ns, &lng, &lngminutes, ew, date, tim) == 8)
    //         {
    //             float lattitude = (float)lat + (latminutes/60);
    //             float longitude = (float)lng + (lngminutes/60);
    //             char* track_points = "        <trkpt lat=\"%f\" lon=\"%f\">\n"
    //                                  "            <ele>0</ele>\n"
    //                                  "            <time>%sT%s</time>\n"
    //                                  "        </trkpt>\n";
    //             if (ns[0] == 'S')
    //                 lattitude *= -1.0;
    //             if (ew[0] == 'W')
    //                 longitude *= -1.0;
    //             fprintf(gpx, track_points, lattitude, longitude, date, tim);
    //             // printf("lat:  %f\n", (float)lat + (latminutes/60));
    //             // printf("lng:  %f\n", (float)lng + (lngminutes/60));
    //             // printf("date: %s\n", date);
    //             // printf("time: %s\n", tim);
    //         }
    //     }
    //     else if (strcmp("Type = Route\r\n", line) == 0)
    //     {
    //         char* track_header = "<trk>\n"
    //                              "    <name>Example GPX Document</name>\n"
    //                              "    <trkseg>\n";
    //         fprintf(gpx, "%s", track_header);
    //         route_started = 1;
    //     }
    // }
}

int main(int argc, char** argv)
{
    FILE* export = 0;
    FILE* gpx = 0;

    if (argc != 3)
    {
        printf("Usage: togpx [text export file] [saved gpx file]\n");
        exit(-1);
    }

    export = fopen(argv[1], "r");
    if (export == 0)
    {
        printf("Error opening text export file. Error #: %i\n", errno);
        exit(-1);
    }

    gpx = fopen(argv[2], "w");
    if (export == 0)
    {
        printf("Error opening saved gpx file. Error #: %i\n", errno);
        exit(-1);
    }

    write_garmin_headers(gpx);
    write_metadata(gpx);

    if (is_tracks(export))
        readwrite_tracks(export, gpx);
    else if (is_routes(export))
        readwrite_routes(export, gpx);
    
    write_footer(gpx);

    if (export != 0)
        fclose(export);
    if (gpx != 0)
        fclose(gpx);
}