% 打开文件
fileID = fopen('data1.log', 'r');

% 读取文件内容
data = textscan(fileID, '%f');

% 关闭文件
fclose(fileID);

% 将数据存储到矩阵中
data_matrix = [data{1}];

% 绘制原始数据图像
figure;
plot(data_matrix(:, 1));
title('Original Data');
xlabel('Index');
ylabel('Value');

% 调用滤波函数
Hd = test;  % 获取滤波器对象
filtered_data = filter(Hd, data_matrix);  % 滤波

% 绘制滤波后的数据图像
figure;
plot(filtered_data);
title('Filtered Data');
xlabel('Index');
ylabel('Value');
