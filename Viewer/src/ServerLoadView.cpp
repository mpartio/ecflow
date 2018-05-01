//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "ServerLoadView.hpp"

#include "File_r.hpp"
#include "File.hpp"
#include "NodePath.hpp"
#include "Str.hpp"
#include "UiLog.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
//#include <QChartView>
#include <QLineSeries>

#include "UIDebug.hpp"

//using namespace QtCharts;

bool sortVecFunction(const std::pair<size_t,size_t>& a, const std::pair<size_t,size_t>& b)
{
    return a.second < b.second;
}

void LogLoadDataItem::clear()
{
    sumTotal_=0;
    maxTotal_=0;
    rank_=-1;
    childReq_.clear();
    userReq_.clear();
    name_.clear();
}

void LogLoadDataItem::init(size_t num)
{
    sumTotal_=0;
    maxTotal_=0;
    rank_=-1;
    childReq_=std::vector<int>(num,0);
    userReq_=std::vector<int>(num,0);
}

void LogLoadData::clear()
{
    time_.clear();
    data_.clear();
    suiteData_.clear();
    suites_.clear();
}

void LogLoadData::setTimeRes(TimeRes res)
{
    timeRes_=res;
}

void LogLoadData::getSeries(QLineSeries& series,const std::vector<int>& vals)
{
    UI_ASSERT(time_.size() == vals.size(), "time_.size()=" << time_.size() << "vals.size()=" << vals.size());

    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
            series.append(time_[i], vals[i]);
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=vals[i];
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                    series.append(time_[i],sum);

                currentMinute=minute;
                sum=0;
            }
        }
    }
}

void LogLoadData::getSeries(QLineSeries& series,const std::vector<int>& vals1,const std::vector<int>& vals2)
{
    UI_ASSERT(time_.size() == vals1.size(), "time_.size()=" << time_.size() << "vals1.size()=" << vals1.size());
    UI_ASSERT(time_.size() == vals2.size(), "time_.size()=" << time_.size() << "vals2.size()=" << vals2.size());

    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
            series.append(time_[i], vals1[i] + vals2[i]);
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=vals1[i] + vals2[i];
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                    series.append(time_[i],sum);

                currentMinute=minute;
                sum=0;
            }
        }
    }
}


void LogLoadData::getChildReq(QLineSeries& series)
{
    getSeries(series,data_.childReq());

#if 0
    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
            series.append(time_[i], data_[i].child_);
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=data_[i].child_;
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                    series.append(time_[i],sum);

                currentMinute=minute;
                sum=0;
            }
        }
    }
#endif
}

void LogLoadData::getUserReq(QLineSeries& series)
{
    getSeries(series,data_.userReq());

#if 0
    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
            series.append(time_[i], data_[i].user_);
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=data_[i].user_;
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                    series.append(time_[i],sum);

                currentMinute=minute;
                sum=0;
            }
        }
    }
#endif

}

void LogLoadData::getTotalReq(QLineSeries& series,int& maxVal)
{
    getSeries(series,data_.childReq(),data_.userReq());
    maxVal=data_.maxTotal();

#if 0
    if(timeRes_ == SecondResolution)
    {
        for(size_t i=0; i < time_.size(); i++)
        {
            int val=data_[i].user_ + data_[i].child_;
            series.append(time_[i], val);
            if(maxVal < val)
                maxVal=val;
         }
    }
    else if(timeRes_ == MinuteResolution)
    {
        qint64 currentMinute=0;
        int sum=0;
        for(size_t i=0; i < time_.size(); i++)
        {
            qint64 minute=time_[i]/60000;
            sum+=data_[i].user_ + data_[i].child_;
            if(currentMinute != minute)
            {
                if(currentMinute >0 )
                {
                    if(maxVal < sum)
                        maxVal=sum;

                    series.append(time_[i],sum);
                }

                currentMinute=minute;
                sum=0;
            }
        }
    }
#endif

}

void LogLoadData::getSuiteReq(QString suiteName,QLineSeries& series)
{
#if 0
    int idx=suiteNames_.indexOf(suiteName);
    if(idx >=0)
    {
        for(int i=0; i < time_.count(); i++)
            series.append(time_[i].toMSecsSinceEpoch(), suite_[idx][i]);
    }
#endif
}

void LogLoadData::add(std::vector<std::string> time_stamp,size_t child_requests_per_second,
                   size_t user_requests_per_second,std::vector<SuiteLoad>& suite_vec)
{
    if(time_stamp.size() < 2)
        return;

    QString s=QString::fromStdString(time_stamp[0]) + " " +
            QString::fromStdString(time_stamp[1]);
    time_.push_back(QDateTime::fromString(s,"HH:mm:ss d.M.yyyy").toMSecsSinceEpoch());

    //all data
    data_.add(child_requests_per_second,user_requests_per_second);

    //suite specific data
    for(size_t i = suiteData_.size(); i < suite_vec.size(); i++)
    {
        suiteData_.push_back(LogLoadDataItem(suite_vec[i].name_));
        suiteData_.back().init(time_.size());
    }

    assert(suiteData_.size() == suite_vec.size());

    //suite specific data
    for(size_t i = 0; i < suiteData_.size(); i++)
    {
        suiteData_[i].add(suite_vec[i].childReq_,
                          suite_vec[i].userReq_);
    }
}

void LogLoadData::processSuites()
{
    if(suiteData_.size() == 0)
        return;

    if(suiteData_.size() == 1)
    {
        suiteData_[0].setRank(0);
        suiteData_[0].setPercentage(100.);
        return;
    }

    std::vector<size_t> sumVec;
    std::vector<std::pair<size_t,size_t> > sortVec;
    size_t sum=0;
    for(size_t i = 0; i < suiteData_.size(); i++)
    {
        sum+=suiteData_[i].sumTotal();
        sumVec.push_back(suiteData_[i].sumTotal());
        sortVec.push_back(std::make_pair(i,suiteData_[i].sumTotal()));
    }

    if(sum <=0 )
        return;

    assert(sumVec.size() == suiteData_.size());
    std::sort(sortVec.begin(), sortVec.end(),sortVecFunction);

    for(size_t i = 0; i < sortVec.size(); i++)
    {
        int idx=sortVec[i].first;
        suiteData_[idx].setRank(i);
        suiteData_[idx].setPercentage(static_cast<float>(suiteData_[idx].sumTotal()*100)/static_cast<float>(sum));
    }
}

void LogLoadData::loadLogFile(const std::string& logFile )
{
   /// Will read the log file.
   /// We will collate each request/cmd to the server made over a second.
   /// There are two kinds of commands:
   ///   o User Commands: these start with --
   ///   o Child Command: these start with chd:
   /// All child commands specify a path and hence suite, whereas for user commands this is optional
   /// We will trap all use of paths, so that we can show which suites are contributing to the server load
   /// This will be done for 4 suites
   ///
   /// Will convert: FROM:
   ///   XXX:[HH:MM:SS D.M.YYYY] chd:init [+additional information]
   ///   XXX:[HH:MM:SS D.M.YYYY] --begin  [+additional information]
   /// -------------:

    //Clear all collected data
    clear();

    //A collector fr suite related data
    std::vector<SuiteLoad> suite_vec;

    /// The log file can be massive > 50Mb
    ecf::File_r log_file(logFile);
    if( !log_file.ok() )
        throw std::runtime_error("LogLoadData::loadLogFile: Could not open log file " + logFile );

    std::vector<std::string> new_time_stamp;
    std::vector<std::string> old_time_stamp;
    size_t childReq = 0, userReq = 0 ;
    std::string line;

    while ( log_file.good() )
    {
        log_file.getline( line); // default delimiter is /n

        // The log file format we are interested is :
        // 0             1         2            3
        // MSG:[HH:MM:SS D.M.YYYY] chd:fullname [path +additional information]
        // MSG:[HH:MM:SS D.M.YYYY] --begin      [args | path(optional) ]    :<user>

        /// We are only interested in Commands (i.e MSG:), and not state changes
        if (line.empty())
            continue;

        if (line[0] != 'M')
            continue;

        std::string::size_type msg_pos = line.find("MSG:");
        if (msg_pos != 0)
            continue;

        bool child_cmd = false;
        bool user_cmd = false;
        if (line.find(ecf::Str::CHILD_CMD()) != std::string::npos)
        {
            child_cmd = true;
        }
        else if (line.find(ecf::Str::USER_CMD()) != std::string::npos)
        {
            user_cmd = true;
        }

        if(!child_cmd && !user_cmd)
            continue;

        new_time_stamp.clear();
        {
            /// MSG:[HH:MM:SS D.M.YYYY] chd:fullname [+additional information] ---> HH:MM:SS D.M.YYYY
            /// EXTRACT the date
            std::string::size_type first_open_bracket = line.find('[');
            if ( first_open_bracket == std::string::npos)
            {
                //std::cout << line << "\n";
                assert(false);
                continue;
            }
            line.erase(0,first_open_bracket+1);

            std::string::size_type first_closed_bracket = line.find(']');
            if ( first_closed_bracket ==  std::string::npos)
            {
                //std::cout << line << "\n";
                assert(false);
                continue;
            }
            std::string time_stamp = line.substr(0, first_closed_bracket);

            ecf::Str::split(time_stamp, new_time_stamp);
            if (new_time_stamp.size() != 2)
                continue;

         line.erase(0,first_closed_bracket+1);
      }

      // Should be just left with " chd:<child command> " or " --<user command>, since we have remove time stamp
//#ifdef DEBUG
//      std::cout << line << "\n";
//#endif

      if (old_time_stamp.empty())
      {
         if (child_cmd)
             childReq++;
         else
             userReq++;

         // Extract path if any, to determine the suite most contributing to server load
         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }
      else if (old_time_stamp[0] == new_time_stamp[0])
      { // HH:MM:SS == HH:MM:SS
         if (child_cmd)
             childReq++;
         else
             userReq++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }



      else {
         /// Start of *NEW* time,
         /// write the *OLD* time line should contain time date without []
         ///    1         2         3             4              5            6        7      8       9       10
         ///  HH:MM:SS D.M.YYYY total_request child_request  users_requests suite_0 suite_1 suite_2 suite_3 suite_n
#if 0
         //plot.add(old_time_stamp[0],old_time_stamp[1],child_requests_per_second, user_request_per_second,


         plot_data_line_number++;
         gnuplot_file << old_time_stamp[0] << " "
                      << old_time_stamp[1] << " "
                      << (child_requests_per_second + user_request_per_second) << " "
                      << child_requests_per_second << " "
                      << user_request_per_second << " ";
         for(size_t i = 0; i < suite_vec.size(); i++) { gnuplot_file << suite_vec[i].request_per_second_ << " ";}
         gnuplot_file << "\n";
#endif

         add(old_time_stamp,childReq,userReq,suite_vec);

         // clear request per second
         childReq = 0;
         userReq = 0;
         for(size_t i= 0; i < suite_vec.size();i++)
         {
            suite_vec[i].childReq_ = 0;
            suite_vec[i].userReq_ = 0;
         }

         // start of *new* time
         if (child_cmd)
             childReq++;
         else
             userReq++;

         size_t column_index = 0;
         bool suite_path_found = extract_suite_path(line,child_cmd,suite_vec,column_index);
         //if ( suite_path_found )
         //    assert(suite_vec[column_index].request_per_second_ <= (child_requests_per_second + user_request_per_second) );
      }

      old_time_stamp = new_time_stamp;
   }

   //if (plot_data_line_number < 3) {
   //   throw std::runtime_error( "Gnuplot::prepare_for_gnuplot: Log file empty or not enough data for plot\n");
   //}

   for(size_t i=0; i < suite_vec.size(); i++)
   {
       suites_ << QString::fromStdString(suite_vec[i].name_);
   }

   processSuites();
}

bool LogLoadData::extract_suite_path(
         const std::string& line,
         bool child_cmd,
         std::vector<SuiteLoad>& suite_vec,
         size_t& column_index   // 0 based
         )
{
    // line should either
    //  chd:<childcommand> path
    //  --<user command)   path<optional> :<user>
    size_t forward_slash = line.find('/');
    if ( forward_slash != std::string::npos)
    {
        std::string path;
        if (child_cmd)
        {
            // For labels ignore paths in the label part
            // MSG:[14:55:04 17.10.2013] chd:label progress 'core/nodeattr/nodeAParser' /suite/build/cray/cray_gnu/build_release/test
            if (line.find("chd:label") != std::string::npos)
            {
                size_t last_tick = line.rfind("'");
                if ( last_tick != std::string::npos )
                {
                    size_t the_forward_slash = line.find('/',last_tick);
                    if (the_forward_slash != std::string::npos)
                    {
                        forward_slash = the_forward_slash;
                    }
                }
            }
            path = line.substr(forward_slash);
        }
        else
        {
            // Ignore the --news command, they dont have a path, hence i.e to ignore line like:
            //  MSG:[09:36:05 22.10.2013] --news=1 36506 6  :ma0 [server handle(36508,7) server(36508,7)
            //                     : *Large* scale changes (new handle or suites added/removed) :NEWS]
            //   the /removed was being interpreted as a suite
            if (line.find("--news") != std::string::npos)
                return false;
        }

        // find the space after the path
        size_t space_pos = line.find(" ",forward_slash);
        if (space_pos != std::string::npos &&  space_pos > forward_slash)
        {
            path = line.substr(forward_slash,space_pos-forward_slash);
        }

        if (!path.empty())
        {
            std::vector<std::string> theNodeNames;
            theNodeNames.reserve(4);
            NodePath::split(path,theNodeNames);
            if (!theNodeNames.empty())
            {
                for(size_t n = 0; n < suite_vec.size(); n++)
                {
                    if (suite_vec[n].name_ == theNodeNames[0] )
                    {
                        if(child_cmd)
                            suite_vec[n].childReq_++;
                        else
                            suite_vec[n].userReq_++;

                        //suite_vec[n].total_request_per_second_++;
                        column_index = n;
                        return true;
                    }
                }

                suite_vec.push_back( SuiteLoad(theNodeNames[0]) );
                column_index = suite_vec.size() - 1;
                if(child_cmd)
                    suite_vec[column_index].childReq_++;
                else
                    suite_vec[column_index].userReq_++;

                return true;
         }
      }
   }
   return false;
}






ChartView::ChartView(QChart *chart, QWidget *parent) :
    QChartView(chart, parent)
{
    setRubberBand(QChartView::HorizontalRubberBand);
}

void ChartView::mousePressEvent(QMouseEvent *event)
{
    QChartView::mousePressEvent(event);
}

void ChartView::mouseMoveEvent(QMouseEvent *event)
{
    QChartView::mouseMoveEvent(event);
}

void ChartView::mouseReleaseEvent(QMouseEvent *event)
{
    QPointF oriLeft=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF oriRight=chart()->mapToValue(chart()->plotArea().topRight());

    //UiLog().dbg() << "  " << chart()->mapToValue(chart()->plotArea().bottomLeft());

    QChartView::mouseReleaseEvent(event);

    //UiLog().dbg() << "   " << chart()->plotArea();
    //UiLog().dbg() << "  " << chart()->mapToValue(chart()->plotArea().bottomLeft());

    QPointF newLeft=chart()->mapToValue(chart()->plotArea().bottomLeft());
    QPointF newRight=chart()->mapToValue(chart()->plotArea().topRight());

    if(newLeft != oriLeft || newRight != oriRight )
    {
        Q_EMIT chartZoomed(QRectF(newLeft,newRight));
    }

    qint64 period=newRight.x()-newLeft.x(); //in ms
    adjustTimeAxis(period);
}

void ChartView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Plus:
        chart()->zoomIn();
        break;
    case Qt::Key_Minus:
        chart()->zoomOut();
        break;
    case Qt::Key_Left:
        chart()->scroll(-10, 0);
        break;
    case Qt::Key_Right:
        chart()->scroll(10, 0);
        break;
    case Qt::Key_Up:
        chart()->scroll(0, 10);
        break;
    case Qt::Key_Down:
        chart()->scroll(0, -10);
        break;
    default:
        QGraphicsView::keyPressEvent(event);
        break;
    }
}

void ChartView::doZoom(QRectF valRect)
{
    QRectF r(chart()->mapToPosition(valRect.bottomLeft()),
             chart()->mapToPosition(valRect.topRight()));

    if(r.isValid())
    {
        chart()->zoomIn(r);
        qint64 period=valRect.width(); //in ms
        adjustTimeAxis(period);
    }
}

void ChartView::adjustTimeAxis(qint64 periodInMs)
{
    qint64 period=periodInMs/1000; //in seconds
    QString format;

    if(period < 60)
    {
        format="hh:mm:ss";
    }
    else if(period < 3600)
    {
        format="hh:mm";
    }
    else if(period < 6*3600)
    {
        format="hh:mm dd/MM";
    }
    else if(period < 12*3600)
    {
        format="hh:mm dd/MM";
    }
    else if(period < 24*3600)
    {
        format="hh:mm dd/MM";
    }
    else if(period < 24*5*3600)
    {
        format="hh dd/MM";
    }
    else
    {
        format="DD/MM";
    }

    if(QDateTimeAxis *ax=static_cast<QDateTimeAxis*>(chart()->axisX()))
    {
        ax->setFormat(format);
    }
}

ServerLoadView::ServerLoadView(QWidget* parent) : QWidget(parent), data_(NULL)
{
    QVBoxLayout* hb=new QVBoxLayout(this);


    chart_ = new QChart();
    chartView_=new ChartView(chart_,this);
    chartView_->setRenderHint(QPainter::Antialiasing);
    hb->addWidget(chartView_);


    chartUserReq_ = new QChart();
    ChartView* chartViewUser=new ChartView(chartUserReq_,this);
    chartViewUser->setRenderHint(QPainter::Antialiasing);
    hb->addWidget(chartViewUser);

    chartChildReq_ = new QChart();
    ChartView* chartViewChild=new ChartView(chartChildReq_,this);
    chartViewChild->setRenderHint(QPainter::Antialiasing);
    hb->addWidget(chartViewChild);


    views_ << chartView_ << chartViewUser << chartViewChild;

    Q_FOREACH(ChartView* v,views_)
    {
        connect(v,SIGNAL(chartZoomed(QRectF)),
                this,SLOT(slotZoom(QRectF)));
    }
}

void ServerLoadView::setData(LogLoadData* data)
{
    assert(data_ == 0);
    data_=data;
}

void ServerLoadView::slotZoom(QRectF r)
{
    if(ChartView* senderView=static_cast<ChartView*>(sender()))
    {
        Q_FOREACH(ChartView* v,views_)
        {
           if(v != senderView)
               v->doZoom(r);
        }
    }
}

void ServerLoadView::setResolution(LogLoadData::TimeRes res)
{
    data_->setTimeRes(res);
    load();
}

void ServerLoadView::load(const std::string& logFile)
{
    data_->loadLogFile(logFile);
    load();

#if 0
    //QChart *chart = new QChart();
    chart_->addSeries(tSeries);
    //chart_->addSeries(chSeries);
    //chart_->addSeries(usSeries);

    //chart_->legend()->hide();
    chart_->setTitle("Server load");

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("HH dd/MM");
    axisX->setTitleText("Date");
    chart_->setAxisX(axisX, tSeries);

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);


    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Requests per second");
    axisY->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chart_->setAxisY(axisY, tSeries);
    axisY->setMin(0.);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);

    chartU_->addSeries(chSeries);

    //chart_->legend()->hide();
    chart_->setTitle("Server load - child command");

    QDateTimeAxis *axisXU = new QDateTimeAxis;
    axisXU->setTickCount(10);
    axisXU->setFormat("HH dd/MM");
    axisXU->setTitleText("Date");
    chartU_->setAxisX(axisXU, chSeries);

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);


    QValueAxis *axisYU = new QValueAxis;
    axisYU->setLabelFormat("%i");
    axisYU->setTitleText("Requests per second");
    axisYU->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chartU_->setAxisY(axisYU, chSeries);
    axisYU->setMin(0.);
    axisYU->setMax(171.);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);
#endif

}

void ServerLoadView::load()
{
    chart_->removeAllSeries();
    chartChildReq_->removeAllSeries();
    chartUserReq_->removeAllSeries();

    chart_->removeAxis(chart_->axisX());
    chart_->removeAxis(chart_->axisY());

    chartChildReq_->removeAxis(chartChildReq_->axisX());
    chartChildReq_->removeAxis(chartChildReq_->axisY());

    chartUserReq_->removeAxis(chartUserReq_->axisX());
    chartUserReq_->removeAxis(chartUserReq_->axisY());

    QLineSeries* chSeries=new QLineSeries();
    data_->getChildReq(*chSeries);

    QLineSeries* usSeries=new QLineSeries();
    data_->getUserReq(*usSeries);

    int maxVal=0;
    QLineSeries* tSeries=new QLineSeries();
    data_->getTotalReq(*tSeries,maxVal);

    build(chart_,tSeries,maxVal);
    build(chartChildReq_,chSeries,maxVal);
    build(chartUserReq_,usSeries,maxVal);
}


void  ServerLoadView::build(QChart* chart,QLineSeries *series, int maxVal)
{
    chart->addSeries(series);

    chart->setTitle("Server load - child command");

    chart->legend()->hide();
    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(10);
    axisX->setFormat("HH dd/MM");
    axisX->setTitleText("Date");
    chart->setAxisX(axisX, series);

    //chart_->addAxis(axisX, Qt::AlignBottom);
    //chSeries->attachAxis(axisX);
    //usSeries->attachAxis(axisX);
    //tSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Requests per second");
    axisY->setMin(0.);
    //chart_->addAxis(axisY, Qt::AlignLeft);
    chart->setAxisY(axisY, series);
    axisY->setMin(0.);
    axisY->setMax(maxVal);
    //chSeries->attachAxis(axisY);
    //usSeries->attachAxis(axisY);
    //tSeries->attachAxis(axisY);

    //connect(series, &QLineSeries::hovered, this, &View::tooltip);
}
#if 0
void View::tooltip(QPointF point, bool state)
{
    if (m_tooltip == 0)
        m_tooltip = new Callout(m_chart);

    if (state) {
        m_tooltip->setText(QString("X: %1 \nY: %2 ").arg(point.x()).arg(point.y()));
        m_tooltip->setAnchor(point);
        m_tooltip->setZValue(11);
        m_tooltip->updateGeometry();
        m_tooltip->show();
    } else {
        m_tooltip->hide();
    }
}
#endif
