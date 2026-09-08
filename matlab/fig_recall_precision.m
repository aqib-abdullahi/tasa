%% Figure: Transient capture recall and precision

labels = {'TASA (detector only)', 'TASA (full FSM)', 'EcoTrack (repurposed)'};
recall    = [97.5, 100, 0];
precision = [0,    71.4, 0];       % 0 used as placeholder for N/A entries
has_precision = [false, true, false];  % track which bars get a real label

figure('Position', [100 100 650 400]);
x = 1:3;
width = 0.35;

hold on;
bR = bar(x - width/2, recall, width, 'FaceColor', [0.18 0.33 0.59]);
bP = bar(x + width/2, precision, width, 'FaceColor', [0.56 0.67 0.86]);
hold off;

set(gca, 'XTick', x, 'XTickLabel', labels);
ylabel('Percent (%)');
title('Transient Capture Recall and Precision');
ylim([0 118]);
legend([bR bP], {'Recall', 'Precision'}, 'Location', 'northeast', 'Box', 'off');
box off;

% Value labels
for i = 1:3
    text(x(i) - width/2, recall(i) + 3, sprintf('%.1f%%', recall(i)), ...
        'HorizontalAlignment', 'center', 'FontSize', 9);
    if has_precision(i)
        text(x(i) + width/2, precision(i) + 3, sprintf('%.1f%%', precision(i)), ...
            'HorizontalAlignment', 'center', 'FontSize', 9);
    else
        text(x(i) + width/2, 3, 'N/A', ...
            'HorizontalAlignment', 'center', 'FontSize', 9, ...
            'FontAngle', 'italic', 'Color', [0.5 0.5 0.5]);
    end
end

set(gca, 'FontName', 'Times New Roman', 'FontSize', 11);
exportgraphics(gcf, 'fig_recall_precision.pdf', 'ContentType', 'vector');
exportgraphics(gcf, 'fig_recall_precision.png', 'Resolution', 300);
