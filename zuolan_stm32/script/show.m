% 打开文件
fileID = fopen('data1.log','r');

% 读取文件内容
data = textscan(fileID, '%f');

% 关闭文件
fclose(fileID);

% 将数据存储到矩阵中
data_matrix = [data{1}];

% 创建图形界面
figure;

plot(data_matrix(:, 1));
title('Data Column 1');
xlabel('Index');
ylabel('Value');
