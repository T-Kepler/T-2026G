% 打开文件
fileID = fopen('data1.log','r');

% 读取文件内容
data = textscan(fileID, '%f,%f');

% 关闭文件
fclose(fileID);

% 将数据存储到矩阵中
data_matrix = [data{1}, data{2}];

% 创建图形界面
figure;

% 绘制左边的图
subplot(1, 2, 1); % 1行2列，第1个图
plot(data_matrix(:, 1));
title('Data Column 1');
xlabel('Index');
ylabel('Value');

% 绘制右边的图
subplot(1, 2, 2); % 1行2列，第2个图
plot(data_matrix(:, 2));
title('Data Column 2');
xlabel('Index');
ylabel('Value');
