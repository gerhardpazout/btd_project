# btd_project
Project for the lecture Beyond the Desktop

## Folder Structure

- ESP-Code: The ESP-IDF project is located in `btd_project/ui/hello_world/`
- Server: The Python server is lcoated in `/server_client_communication/server.py`
- Analysis Tools: The final analysis tool is located in `btd_project/data_analysis_tool/tool2.ipynb`
- Data: The test data is stored in `btd_project/server_client_communication/uploads`

## Manual Config

The ESP uses fonts and thus a custom partitions.csv table was setup. By default the ESP does not use it however!
To make sure the ESP uses the partitions.csv open the `menuconfig` and search for `partition table` and in the dropdown 
select the option that says `Custom ...`. Also make sure to increase the flash size! in the `menuconfig` search for `flash size` and make sure `4MB` is selected.