readableFile = fopen('declination.csv', 'w');
binaryFile = fopen('declination.bin', 'w');
day = 1;
month = 1;
year = 2016;
unixvalue = int64(posixtime(datetime(year, month, day)));
fprintf(readableFile, '%i-%i-%i\n', day, month, year);
fwrite(binaryFile, unixvalue, 'uint64');
for lon = -180.0:5.0:180.0
    fprintf(readableFile, ' %.2f', lon);
end
fprintf(readableFile, '\n');
for lat = -90.0:5.0:90.0
    fprintf(readableFile, '%.2f ', lat);
    for lon = -180.0:5.0:180.0
        height = 0; % Calculating values on the 0 meters over geoid
        [XYZ, H, DEC, DIP, F] = wrldmagm(height, lat, lon, decyear(2016,1,1),'2015');
        fprintf(readableFile, '%i ', int16(DEC * 100));
        fwrite(binaryFile, int16(DEC * 100), 'int16');
    end
    fprintf(readableFile, '\n');
end
fclose(readableFile);
fclose(binaryFile);