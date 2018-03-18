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

// Multiple marks make up a single route
#define EXPORT_ROUTE "Type = Route\r\n"
#define EXPORT_MARK "Type = Mark\r\n"
#define EXPORT_MARK_TIME "CreateTime"
#define EXPORT_MARK_COORD "LatLon"

// Multiple trackmarks make up a single track
#define EXPORT_TRACK "Type = Track\r\n"
#define EXPORT_TRACKMARKS_BEGIN "TrackMarks = {{\r\n"
#define EXPORT_TRACKMARKS_END "}}\r\n"

// line buffer size when reading exported text file
#define LINE_BUFFER 1000



struct trackpoint
{
    float lat;
    float lng;
    char timestamp[21];
};


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
    char line[LINE_BUFFER];
    while (fgets(line, LINE_BUFFER, file) != 0)
    {
        if (strcmp(line, str) == 0)
            return 1;
    }
    return 0;
}

int starts_with(char* str, char* find)
{
    int len = strlen(find);
    for (int i = 0; i < len; ++i)
    {
        if (find[i] != str[i])
            return 0;
    }
    return 1;
}

void readwrite_tracks(FILE* export, FILE* gpx)
{
    char line[LINE_BUFFER];
    int start = 0;

    while (fgets(line, LINE_BUFFER, export) != 0)
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

            if (strcmp(EXPORT_TRACKMARKS_END, line) == 0)
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
        else if (strcmp(EXPORT_TRACKMARKS_BEGIN, line) == 0)
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
    char* track_header = "<trk>\n"
                         "    <name>Example GPX Document</name>\n"
                         "    <trkseg>\n";
    char* track_point  = "        <trkpt lat=\"%f\" lon=\"%f\">\n"
                         "            <ele>0</ele>\n"
                         "            <time>%s</time>\n"
                         "        </trkpt>\n";
   char* track_footer  = "    </trkseg>\n"
                         "</trk>\n";

    char line[LINE_BUFFER];
    struct trackpoint point;
    int route_started = 0;
    int ignore_next_time = 1;

    while (fgets(line, LINE_BUFFER, export) != 0)
    {
        if (strcmp(EXPORT_ROUTE, line) == 0)
        {
            if (route_started)
                fprintf(gpx, "%s", track_footer);
            else
                route_started = 1;

            ignore_next_time = 1;
            fprintf(gpx, "%s", track_header);
        }
        // start of mark
        else if (starts_with(line, EXPORT_MARK_TIME))
        {
            char throwaway[LINE_BUFFER];
            char date[11];
            char time[10];

            if (ignore_next_time)
            {
                ignore_next_time = 0;
                continue;
            }

            // e.g. CreateTime = 2017-05-13 22:19:00Z
            if (sscanf(line, "%s %s %s %s", throwaway, throwaway, date, time) == 4)
                sprintf(point.timestamp, "%sT%s", date, time);
        }
        // end of mark
        else if (starts_with(line, EXPORT_MARK_COORD))
        {
            char throwaway[LINE_BUFFER];
            int lat;
            float latmin;
            char ns[2]; // N or S
            int lng;
            float lngmin;
            char ew[2]; // E or W
            
            // e.g. LatLon = 50 38.11920 N 126 17.97594 W
            if (sscanf(line, "%s %s %i %f %s %i %f %s", throwaway, throwaway, &lat, &latmin, ns, &lng, &lngmin, ew) == 8)
            {
                int latsign = (ns[0] == 'N') ? 1 : -1;
                int lngsign = (ns[0] == 'E') ? 1 : -1;
                point.lat = (float)lat * latsign + (latmin/60);
                point.lng = (float)lng * lngsign + (lngmin/60);

                fprintf(gpx, track_point, point.lat, point.lng, point.timestamp);
            }
        }
    }

    if (route_started)
        fprintf(gpx, "%s", track_footer);
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
    if (gpx == 0)
    {
        fclose(export);
        printf("Error opening saved gpx file. Error #: %i\n", errno);
        exit(-1);
    }

    write_garmin_headers(gpx);
    write_metadata(gpx);

    if (contains_line(export, EXPORT_TRACK))
        readwrite_tracks(export, gpx);
    else if (fseek(export, 0, SEEK_SET) == 0 && contains_line(export, EXPORT_ROUTE))
        readwrite_routes(export, gpx);
    else
        printf("Non supported export file.\n");
    
    write_footer(gpx);

    if (export != 0)
        fclose(export);
    if (gpx != 0)
        fclose(gpx);
}