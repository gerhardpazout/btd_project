# Menuconfig Settings

Because we are using fonts for the display we need to set the correct partition table in the menuconfig!

1. Go to menuconfig (the cog wheel at the bottom bar in VS Code)
2. Type "Partition table"
3. Select "Custom Partition Table CSV" from the drop down
4. Make sure the field below called "Custom partition CSV file" has the value "partitions.csv" (default)
5. In menuconfig search for "Flash Size" and set it to 4MB
6. Save
7. Build
