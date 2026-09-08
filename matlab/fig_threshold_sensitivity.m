%% Figure: Threshold sensitivity analysis (recall and precision heatmaps)
% Data below is the actual measured sweep: for every combination of
% tau_slope and tau_rms, the full TASA detector+hysteresis pipeline was
% re-run against the labeled synthetic signal and recall/precision
% recorded. Chosen operating point (tau_slope=40, tau_rms=60) is boxed.

tau_slope = [20 30 40 60 100 200 300 400 500];
tau_rms   = [30 60 90 150 250 350 450];

% Rows = tau_rms, Columns = tau_slope (matches heatmap orientation)
recall = [
    1.0000 1.0000 1.0000 1.0000 1.0000 0.9875 0.9875 0.9875 0.9750;
    1.0000 1.0000 1.0000 1.0000 1.0000 0.9750 0.9750 0.9750 0.9500;
    1.0000 1.0000 1.0000 1.0000 1.0000 0.9625 0.9625 0.9625 0.9375;
    1.0000 1.0000 1.0000 1.0000 1.0000 0.9250 0.9250 0.9250 0.8750;
    0.8375 0.7625 0.7625 0.7625 0.7625 0.6250 0.6250 0.6250 0.5375;
    0.8375 0.7625 0.7625 0.7625 0.7625 0.6250 0.6250 0.6250 0.5125;
    0.3500 0.2750 0.2750 0.2750 0.2750 0.1375 0.1375 0.1375 0.0000
];

precision = [
    0.2899 0.6897 0.6897 0.6897 0.6897 0.6870 0.6870 0.6870 0.6842;
    0.2909 0.7018 0.7018 0.7018 0.7018 0.6964 0.6964 0.6964 0.6909;
    0.2920 0.7143 0.7143 0.7143 0.7143 0.7064 0.7064 0.7064 0.7009;
    0.2930 0.7407 0.7407 0.7407 0.7407 0.7255 0.7255 0.7255 0.7143;
    0.2627 0.7262 0.7262 0.7262 0.7262 0.6849 0.8065 0.8065 0.7818;
    0.2638 0.7349 0.7349 0.7349 0.7349 0.6944 0.8197 0.8197 0.8039;
    0.1302 0.5000 0.5000 0.5000 0.5000 0.3333 0.5000 0.5000 0.0000
];

% Indices of the chosen operating point (tau_slope=40, tau_rms=60)
chosen_col = find(tau_slope == 40);
chosen_row = find(tau_rms == 60);

figure('Position', [100 100 1000 420]);

% ---- Recall subplot ----
subplot(1,2,1);
imagesc(recall);
colormap(gca, flipud(redgreencmap()));  % see helper function below
caxis([0 1]);
colorbar;
set(gca, 'XTick', 1:length(tau_slope), 'XTickLabel', tau_slope, ...
         'YTick', 1:length(tau_rms), 'YTickLabel', tau_rms);
xlabel('\tau_{slope}');
ylabel('\tau_{rms}');
title('Recall');
hold on;
rectangle('Position', [chosen_col-0.5, chosen_row-0.5, 1, 1], ...
          'EdgeColor', 'b', 'LineWidth', 2.5);
% Print values in each cell
for r = 1:size(recall,1)
    for c = 1:size(recall,2)
        text(c, r, sprintf('%.2f', recall(r,c)), ...
            'HorizontalAlignment', 'center', 'FontSize', 7);
    end
end
hold off;

% ---- Precision subplot ----
subplot(1,2,2);
imagesc(precision);
colormap(gca, flipud(redgreencmap()));
caxis([0 1]);
colorbar;
set(gca, 'XTick', 1:length(tau_slope), 'XTickLabel', tau_slope, ...
         'YTick', 1:length(tau_rms), 'YTickLabel', tau_rms);
xlabel('\tau_{slope}');
ylabel('\tau_{rms}');
title('Precision');
hold on;
rectangle('Position', [chosen_col-0.5, chosen_row-0.5, 1, 1], ...
          'EdgeColor', 'b', 'LineWidth', 2.5);
for r = 1:size(precision,1)
    for c = 1:size(precision,2)
        text(c, r, sprintf('%.2f', precision(r,c)), ...
            'HorizontalAlignment', 'center', 'FontSize', 7);
    end
end
hold off;

sgtitle('Threshold Sensitivity: TASA Trigger Detector (blue box = chosen operating point, \tau_{slope}=40, \tau_{rms}=60)');

exportgraphics(gcf, 'fig_threshold_sensitivity.pdf', 'ContentType', 'vector');
exportgraphics(gcf, 'fig_threshold_sensitivity.png', 'Resolution', 300);


%% Helper function: red-green colormap (define at end of script file,
%% or save separately as redgreencmap.m on your MATLAB path)
function cmap = redgreencmap()
    n = 64;
    r = [linspace(0.8,1,n/2), linspace(1,0.1,n/2)];
    g = [linspace(0.1,1,n/2), linspace(1,0.5,n/2)];
    b = [linspace(0.1,0.3,n/2), linspace(0.3,0.1,n/2)];
    cmap = [r' g' b'];
end
