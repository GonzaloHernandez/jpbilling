#ifndef ABOUT
#define ABOUT

#include <QDialog>
#include <QMouseEvent>
#include <QLabel>
#include <QVBoxLayout>

/**
 * @brief The About class
 */
class About : public QDialog
{
    Q_OBJECT
private:
    QLabel* info[6];

    void mousePressEvent(QMouseEvent*)
    {
        close();
    }

public:
    About(QWidget* p) : QDialog(p)
    {
        setWindowTitle("Acerca de esta aplicación");
        QSize fixedSize(300,150);
        resize(fixedSize);
        QVBoxLayout* layout = new QVBoxLayout();
        setLayout(layout);
        info[0] = new QLabel("Universidad de Nariño");
        info[1] = new QLabel("<a href='http://sonar.udenar.edu.co/jpbilling'>http://sonar.udenar.edu.co</a>");
        info[2] = new QLabel("-------------------------------------");
        info[3] = new QLabel("[ Release 59 ]");
        info[4] = new QLabel("GonzaloHernandez@udenar.edu.co");

        QFont font = info[4]->font();
        font.setPointSize(9);

        for (int i=0; i<5; i++)
        {
            info[i]->setFont(font);
            info[i]->setAlignment(Qt::AlignHCenter);
            layout->addWidget(info[i]);
        }

        info[1]->setTextFormat(Qt::RichText);
        info[1]->setTextInteractionFlags(Qt::TextBrowserInteraction);
        info[1]->setOpenExternalLinks(true);

        setMinimumSize(fixedSize);
        setMaximumSize(fixedSize);
        setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

        setModal(true);
    }
};

#endif // ABOUT
