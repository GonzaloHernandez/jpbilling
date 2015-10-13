#ifndef ABOUT
#define ABOUT

#include <QDialog>
#include <QMouseEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>

/**
 * @brief The About class is required to show the author information.  This interface
 * allow to user to follow some important links.
 */
class About : public QDialog
{
    Q_OBJECT
private:
    QLabel* info[6];
    /**
     * @brief mousePressEvent enable to close the current dialog doing click over its
     * surface.
     */
    void mousePressEvent(QMouseEvent*)
    {
        close();
    }

public:
    /**
     * @brief About is the constructor class.  Its aim is arrange the information.
     * @param p needed to join this dialog with the main window.
     */
    About(QWidget* p) : QDialog(p)
    {
        setWindowTitle("Acerca de esta aplicación");
        QSize fixedSize(300,150);
        resize(fixedSize);

        QFile file("/etc/jpbilling/jpbilling.conf");
        QString release = "0";
        if(file.open(QIODevice::ReadOnly)) {
            QTextStream in(&file);
            release = in.readLine();
            file.close();
        }

        QVBoxLayout* layout = new QVBoxLayout();
        setLayout(layout);
        info[0] = new QLabel("Universidad de Nariño");
        info[1] = new QLabel("<a href='http://sonar.udenar.edu.co/jpbilling'>http://sonar.udenar.edu.co</a>");
        info[2] = new QLabel("-------------------------------------");
        info[3] = new QLabel( QString("[ %1 ]").arg(release) );
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

